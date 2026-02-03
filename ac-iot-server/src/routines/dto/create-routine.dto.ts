import { IsString, IsNotEmpty, IsBoolean, IsOptional, IsArray, ValidateNested } from 'class-validator';
import { Type } from 'class-transformer';

export class CreateRoutineDto {
    @IsString()
    @IsNotEmpty()
    name: string;

    @IsBoolean()
    @IsOptional()
    enabled?: boolean;

    @IsString()
    @IsOptional()
    cron?: string;

    @IsArray()
    @IsOptional()
    triggers?: any[];

    @IsArray()
    @IsNotEmpty()
    actions: any[];
}
