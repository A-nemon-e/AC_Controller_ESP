import { IsNumber } from 'class-validator';

export class AcceptAgreementDto {
  @IsNumber()
  agreementId: number;
}
