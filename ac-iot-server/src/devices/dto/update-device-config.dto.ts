import { IsString, IsOptional, IsObject, IsBoolean, IsNumber, Min, Max, ValidateNested, IsIn } from 'class-validator';
import { Type } from 'class-transformer';

export class MicConfigDto {
    @IsBoolean()
    @IsOptional()
    enabled?: boolean;

    @IsNumber()
    @Min(0)
    @Max(100)
    @IsOptional()
    sensitivity?: number;

    @IsNumber()
    @Min(100)
    @Max(5000)
    @IsOptional()
    beepDurationMs?: number; // 提示音持续时间阈值（毫秒）

    @IsString()
    @IsIn(['short', 'long', 'double'])
    @IsOptional()
    beepType?: string; // 提示音类型：短促/长/双响
}

export class UpdateDeviceConfigDto {
    @IsString()
    @IsOptional()
    brandConfig?: string;

    @IsObject()
    @ValidateNested()
    @Type(() => MicConfigDto)
    @IsOptional()
    micConfig?: MicConfigDto;

    @IsBoolean()
    @IsOptional()
    enableCurrent?: boolean; // 电流互感器开关
}
