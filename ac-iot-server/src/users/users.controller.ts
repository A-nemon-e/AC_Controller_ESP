import {
  Controller,
  Get,
  Patch,
  Post,
  Body,
  UseGuards,
  Request,
  BadRequestException,
} from '@nestjs/common';
import { UsersService } from './users.service';
import { JwtAuthGuard } from '../auth/jwt-auth.guard';
import { PasswordService } from '../auth/password.service';
import {
  UpdateUserSettingsDto,
  ChangePasswordDto,
} from './dto/user-settings.dto';

@UseGuards(JwtAuthGuard)
@Controller('users')
export class UsersController {
  constructor(
    private readonly usersService: UsersService,
    private readonly passwordService: PasswordService,
  ) {}

  @Get('me/settings')
  getSettings(@Request() req: any) {
    return this.usersService.getSettings(req.user.userId);
  }

  @Patch('me/settings')
  updateSettings(@Request() req: any, @Body() dto: UpdateUserSettingsDto) {
    return this.usersService.updateSettings(req.user.userId, dto);
  }

  @Post('me/change-password')
  async changePassword(@Request() req: any, @Body() dto: ChangePasswordDto) {
    await this.passwordService.changePassword(
      req.user.userId,
      dto.oldPassword,
      dto.newPassword,
    );
    return { message: 'Password changed successfully' };
  }

  @Get('me/password-status')
  async getPasswordStatus(@Request() req: any) {
    const userId = req.user.userId;
    const expired = await this.passwordService.isPasswordExpired(userId);
    const user = await this.usersService.findById(userId);

    if (!user) {
      throw new BadRequestException('User not found');
    }

    let daysUntilExpiry: number | undefined;
    if (!expired && user.passwordChangedAt) {
      const daysSinceChange = Math.floor(
        (Date.now() - user.passwordChangedAt.getTime()) / (1000 * 60 * 60 * 24),
      );
      const PASSWORD_POLICY = {
        maxAge: 90, // 默认90天
      };
      daysUntilExpiry = PASSWORD_POLICY.maxAge - daysSinceChange;
    }

    return {
      expired,
      daysUntilExpiry,
      lastChanged: user.passwordChangedAt,
    };
  }
}
