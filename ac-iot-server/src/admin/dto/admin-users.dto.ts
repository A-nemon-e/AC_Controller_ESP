import {
  IsString,
  IsOptional,
  IsEnum,
  IsBoolean,
  IsNumber,
  Min,
  Max,
  IsIn,
} from 'class-validator';
import { UserRole } from '../../users/user.entity';

export class ListUsersQueryDto {
  @IsOptional()
  @IsString()
  search?: string;

  @IsOptional()
  @IsEnum(UserRole)
  role?: UserRole;

  @IsOptional()
  @IsBoolean()
  isActive?: boolean;

  @IsOptional()
  @IsNumber()
  @Min(1)
  page?: number = 1;

  @IsOptional()
  @IsNumber()
  @Min(1)
  @Max(100)
  pageSize?: number = 20;

  @IsOptional()
  @IsIn(['createdAt', 'username', 'lastLoginAt'])
  sortBy?: string = 'createdAt';

  @IsOptional()
  @IsIn(['ASC', 'DESC'])
  sortOrder?: 'ASC' | 'DESC' = 'DESC';
}

export class UpdateUserStatusDto {
  @IsBoolean()
  isActive: boolean;

  @IsOptional()
  @IsString()
  reason?: string;
}

export class ResetPasswordDto {
  @IsOptional()
  @IsBoolean()
  requireChange?: boolean;

  @IsOptional()
  @IsString()
  tempPassword?: string;
}

export class DeleteUserDto {
  @IsBoolean()
  transferDevices: boolean;

  @IsOptional()
  @IsNumber()
  transferToUserId?: number;

  @IsString()
  reason: string;
}
