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
    brandConfig?: string;
}
