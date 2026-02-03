import { IsString, IsOptional, IsNumber, IsBoolean } from 'class-validator';

export class AcCommandDto {
    @IsBoolean()
    @IsOptional()
    power?: boolean;

    @IsString()
    @IsOptional()
    mode?: string; // cool, heat, fan, dry, auto

    @IsNumber()
    @IsOptional()
    temp?: number;

    @IsNumber()
    @IsOptional()
    fan?: number;

    @IsBoolean()
    @IsOptional()
    swingVertical?: boolean;

    @IsBoolean()
    @IsOptional()
    swingHorizontal?: boolean;
}
