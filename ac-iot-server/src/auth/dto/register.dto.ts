import { IsString, MinLength, IsBoolean, IsNumber, IsOptional } from 'class-validator';

export class RegisterDto {
  @IsString()
  @MinLength(3)
  username: string;

  @IsString()
  @MinLength(6)
  password: string;

  @IsBoolean()
  @IsOptional()
  agreementAccepted?: boolean;

  @IsNumber()
  @IsOptional()
  agreementId?: number;
}
