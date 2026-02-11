import { IsString, IsNotEmpty, IsBoolean, IsNumber, IsOptional, ValidateNested, IsIn } from 'class-validator';
import { Type } from 'class-transformer';

class ScheduleAction {
    @IsBoolean()
    power: boolean;

    @IsString()
    @IsOptional()
    mode?: string;

    @IsNumber()
    @IsOptional()
    temp?: number;

    @IsString()
    @IsOptional()
    fan?: string;
}

export class CreateScheduleDto {
    @IsString()
    @IsNotEmpty()
    name: string;

    @IsNumber()
    deviceId: number;

    @IsBoolean()
    @IsOptional()
    enabled?: boolean;

    @IsNumber()
    hour: number;

    @IsNumber()
    minute: number;

    @IsString()
    @IsIn(['daily', 'weekdays', 'weekends', 'custom'])
    repeat: 'daily' | 'weekdays' | 'weekends' | 'custom';

    @IsOptional()
    weekdays?: number[];

    @ValidateNested()
    @Type(() => ScheduleAction)
    action: ScheduleAction;
}
