import { IsString, IsNotEmpty, IsOptional, IsArray, ArrayMinSize } from 'class-validator';

export class SetupBrandDto {
    @IsString()
    @IsNotEmpty()
    brandId: string; // 品牌ID，如 'gree', 'midea', 'haier'

    @IsString()
    @IsOptional()
    model?: string; // 可选的型号
}

export class SetupLearnAllDto {
    @IsArray()
    @ArrayMinSize(1)
    keys: string[]; // 需要学习的按键列表，如 ['power_on', 'cool_26', 'heat_30']
}
