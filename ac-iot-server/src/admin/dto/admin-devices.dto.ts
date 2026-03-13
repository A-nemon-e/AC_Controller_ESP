import {
  IsString,
  IsOptional,
  IsNumber,
  IsBoolean,
  Min,
  Max,
  IsIn,
} from 'class-validator';

export class ListDevicesQueryDto {
  @IsOptional()
  @IsString()
  search?: string;

  @IsOptional()
  @IsString()
  status?: string;

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
  @IsIn(['createdAt', 'lastSeen', 'name'])
  sortBy?: string = 'createdAt';

  @IsOptional()
  @IsIn(['ASC', 'DESC'])
  sortOrder?: 'ASC' | 'DESC' = 'DESC';
}

export class ForceUnbindDto {
  @IsString()
  reason: string;

  @IsOptional()
  @IsBoolean()
  notifyUser?: boolean;

  @IsOptional()
  @IsBoolean()
  backupConfig?: boolean;
}

export class DebugCommandDto {
  @IsString()
  command: string;

  @IsOptional()
  payload?: any;
}
