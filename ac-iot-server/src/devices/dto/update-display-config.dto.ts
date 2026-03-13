import {
  IsOptional,
  IsString,
  IsNumber,
  IsBoolean,
  IsArray,
  ValidateNested,
} from 'class-validator';
import { Type } from 'class-transformer';

export class CardConfigDto {
  @IsOptional()
  @IsString()
  id?: string;

  @IsOptional()
  @IsString()
  name?: string;

  @IsOptional()
  @IsString()
  background?: string;

  @IsOptional()
  @IsArray()
  foregrounds?: string[];

  @IsOptional()
  @IsNumber()
  duration?: number;

  @IsOptional()
  @IsString()
  transition?: string;
}

export class UpdateDisplayConfigDto {
  @IsOptional()
  @IsBoolean()
  autoSwitch?: boolean;

  @IsOptional()
  @IsNumber()
  switchInterval?: number;

  @IsOptional()
  @IsString()
  transition?: string;

  @IsOptional()
  @IsNumber()
  brightness?: number;

  @IsOptional()
  @IsBoolean()
  weatherEnabled?: boolean;

  @IsOptional()
  @IsArray()
  @ValidateNested({ each: true })
  @Type(() => CardConfigDto)
  cards?: CardConfigDto[];
}
