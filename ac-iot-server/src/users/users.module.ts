import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { UsersService } from './users.service';
import { User } from './user.entity';
import { UserSettings } from './user-settings.entity';
import { UsersController } from './users.controller';
import { PasswordService } from '../auth/password.service';
import { PasswordHistory } from '../auth/password-history.entity';

@Module({
  imports: [TypeOrmModule.forFeature([User, UserSettings, PasswordHistory])],
  providers: [UsersService, PasswordService],
  controllers: [UsersController],
  exports: [UsersService],
})
export class UsersModule {}
