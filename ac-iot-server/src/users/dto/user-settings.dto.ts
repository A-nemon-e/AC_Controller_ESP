import { IsString, IsIn, IsOptional, IsNotEmpty, MinLength } from 'class-validator';

export class UpdateUserSettingsDto {
    @IsString()
    @IsIn(['C', 'F'])
    @IsOptional()
    temperatureUnit?: string;

    @IsString()
    @IsIn(['dark', 'light'])
    @IsOptional()
    theme?: string;
}

export class ChangePasswordDto {
    @IsString()
    @IsNotEmpty()
    oldPassword: string;

    @IsString()
    @MinLength(6)
    newPassword: string;
}
