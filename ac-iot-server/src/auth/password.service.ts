import {
  Injectable,
  Logger,
  NotFoundException,
  UnauthorizedException,
  BadRequestException,
} from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import * as bcrypt from 'bcrypt';
import { User } from '../users/user.entity';
import { PasswordHistory } from './password-history.entity';
import { PASSWORD_POLICY } from '../common/constants/security';

@Injectable()
export class PasswordService {
  private readonly logger = new Logger(PasswordService.name);

  constructor(
    @InjectRepository(User)
    private userRepository: Repository<User>,
    @InjectRepository(PasswordHistory)
    private passwordHistoryRepository: Repository<PasswordHistory>,
  ) {}

  async changePassword(
    userId: number,
    currentPassword: string,
    newPassword: string,
  ): Promise<void> {
    const user = await this.userRepository.findOne({
      where: { id: userId },
    });

    if (!user) {
      throw new NotFoundException('User not found');
    }

    const isCurrentValid = await bcrypt.compare(currentPassword, user.password);
    if (!isCurrentValid) {
      throw new UnauthorizedException('Current password is incorrect');
    }

    await this.validatePasswordPolicy(newPassword);

    await this.checkPasswordHistory(userId, newPassword);

    const newHash = await bcrypt.hash(newPassword, 10);
    user.password = newHash;
    user.passwordChangedAt = new Date();
    await this.userRepository.save(user);

    await this.passwordHistoryRepository.save({
      userId,
      passwordHash: newHash,
      createdAt: new Date(),
    });

    await this.cleanupPasswordHistory(userId);

    this.logger.log(`Password changed for user ${userId}`);
  }

  private async validatePasswordPolicy(password: string): Promise<void> {
    const errors: string[] = [];

    if (password.length < PASSWORD_POLICY.minLength) {
      errors.push(
        `Password must be at least ${PASSWORD_POLICY.minLength} characters`,
      );
    }

    if (password.length > PASSWORD_POLICY.maxLength) {
      errors.push(
        `Password must not exceed ${PASSWORD_POLICY.maxLength} characters`,
      );
    }

    if (PASSWORD_POLICY.requireUppercase && !/[A-Z]/.test(password)) {
      errors.push('Password must contain at least one uppercase letter');
    }

    if (PASSWORD_POLICY.requireLowercase && !/[a-z]/.test(password)) {
      errors.push('Password must contain at least one lowercase letter');
    }

    if (PASSWORD_POLICY.requireNumbers && !/[0-9]/.test(password)) {
      errors.push('Password must contain at least one number');
    }

    if (
      PASSWORD_POLICY.requireSpecialChars &&
      !/[!@#$%^\u0026*]/.test(password)
    ) {
      errors.push('Password must contain at least one special character');
    }

    if (errors.length > 0) {
      throw new BadRequestException(errors.join('; '));
    }
  }

  private async checkPasswordHistory(
    userId: number,
    newPassword: string,
  ): Promise<void> {
    const history = await this.passwordHistoryRepository.find({
      where: { userId },
      order: { createdAt: 'DESC' },
      take: PASSWORD_POLICY.historyCount,
    });

    for (const record of history) {
      const isMatch = await bcrypt.compare(newPassword, record.passwordHash);
      if (isMatch) {
        throw new BadRequestException(
          `Cannot reuse the last ${PASSWORD_POLICY.historyCount} passwords`,
        );
      }
    }
  }

  private async cleanupPasswordHistory(userId: number): Promise<void> {
    const history = await this.passwordHistoryRepository.find({
      where: { userId },
      order: { createdAt: 'DESC' },
      skip: PASSWORD_POLICY.historyCount,
    });

    if (history.length > 0) {
      await this.passwordHistoryRepository.remove(history);
    }
  }

  async isPasswordExpired(userId: number): Promise<boolean> {
    const user = await this.userRepository.findOne({
      where: { id: userId },
      select: ['passwordChangedAt'],
    });

    if (!user || !user.passwordChangedAt) {
      return false;
    }

    const daysSinceChange = Math.floor(
      (Date.now() - user.passwordChangedAt.getTime()) / (1000 * 60 * 60 * 24),
    );

    return daysSinceChange > PASSWORD_POLICY.maxAge;
  }
}
