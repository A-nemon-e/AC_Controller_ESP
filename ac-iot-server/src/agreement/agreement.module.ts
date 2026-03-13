import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { AgreementService } from './agreement.service';
import { AgreementController } from './agreement.controller';
import { UserAgreement } from './user-agreement.entity';
import { UserAgreementAcceptance } from './user-agreement-acceptance.entity';

@Module({
  imports: [TypeOrmModule.forFeature([UserAgreement, UserAgreementAcceptance])],
  controllers: [AgreementController],
  providers: [AgreementService],
  exports: [AgreementService],
})
export class AgreementModule {}
