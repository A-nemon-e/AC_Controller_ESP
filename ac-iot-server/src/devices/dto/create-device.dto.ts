import { IsString, IsNotEmpty, IsOptional } from 'class-validator';

export class CreateDeviceDto {
    @IsString()
    @IsNotEmpty()
    name: string;

    @IsString()
    @IsNotEmpty()
    uuid: string;

    @IsString()
    @IsOptional()
    @IsString()
    @IsOptional()
    brandConfig?: string;

    @IsString()
    @IsOptional()
    mac?: string;

    @IsString()
    @IsOptional()
    ip?: string;
}
