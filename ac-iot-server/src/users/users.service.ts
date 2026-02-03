import { Injectable, BadRequestException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { User } from './user.entity';
import { UserSettings } from './user-settings.entity';
import * as bcrypt from 'bcrypt';

@Injectable()
export class UsersService {
    constructor(
        @InjectRepository(User)
        private usersRepository: Repository<User>,
        @InjectRepository(UserSettings)
        private settingsRepository: Repository<UserSettings>,
    ) { }

    async findOne(username: string): Promise<User | null> {
        return this.usersRepository.findOne({ where: { username } });
    }

    async create(user: Partial<User>): Promise<User> {
        const newUser = this.usersRepository.create(user);
        return this.usersRepository.save(newUser);
    }

    async getSettings(userId: number): Promise<UserSettings> {
        let settings = await this.settingsRepository.findOne({ where: { userId } });
        if (!settings) {
            // 创建默认设置
            settings = this.settingsRepository.create({ userId });
            await this.settingsRepository.save(settings);
        }
        return settings;
    }

    async updateSettings(userId: number, dto: Partial<UserSettings>): Promise<UserSettings> {
        const settings = await this.getSettings(userId);
        Object.assign(settings, dto);
        return this.settingsRepository.save(settings);
    }

    async changePassword(userId: number, dto: { oldPassword: string; newPassword: string }): Promise<void> {
        const user = await this.usersRepository.findOne({ where: { id: userId } });
        if (!user) throw new BadRequestException('User not found');

        // 验证旧密码
        const valid = await bcrypt.compare(dto.oldPassword, user.password);
        if (!valid) throw new BadRequestException('Invalid old password');

        // 更新密码
        const salt = await bcrypt.genSalt();
        user.password = await bcrypt.hash(dto.newPassword, salt);
        await this.usersRepository.save(user);
    }
}
