import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { UserAgreement } from './user-agreement.entity';
import { UserAgreementAcceptance } from './user-agreement-acceptance.entity';

@Injectable()
export class AgreementService {
  constructor(
    @InjectRepository(UserAgreement)
    private agreementRepository: Repository<UserAgreement>,
    @InjectRepository(UserAgreementAcceptance)
    private acceptanceRepository: Repository<UserAgreementAcceptance>,
  ) {}

  async getCurrentAgreement(): Promise<UserAgreement | null> {
    return this.agreementRepository.findOne({
      where: { isCurrent: true },
      order: { effectiveDate: 'DESC' },
    });
  }

  async hasUserAccepted(userId: number): Promise<boolean> {
    const currentAgreement = await this.getCurrentAgreement();
    if (!currentAgreement) return true;

    const acceptance = await this.acceptanceRepository.findOne({
      where: {
        userId,
        agreementId: currentAgreement.id,
      },
    });

    return !!acceptance;
  }

  async acceptAgreement(
    userId: number,
    agreementId: number,
    ipAddress: string,
    userAgent: string,
  ): Promise<void> {
    await this.acceptanceRepository.save({
      userId,
      agreementId,
      ipAddress,
      userAgent,
      acceptedAt: new Date(),
    });
  }

  async publishAgreement(
    version: string,
    content: string,
    changes: string[],
    effectiveDate?: Date,
  ): Promise<UserAgreement> {
    await this.agreementRepository.update(
      { isCurrent: true },
      { isCurrent: false },
    );

    return this.agreementRepository.save({
      version,
      content,
      changes,
      isCurrent: true,
      effectiveDate: effectiveDate || new Date(),
    });
  }
}
