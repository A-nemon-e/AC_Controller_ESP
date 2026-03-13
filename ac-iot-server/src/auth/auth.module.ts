import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { AuthService } from './auth.service';
import { UsersModule } from '../users/users.module';
import { PassportModule } from '@nestjs/passport';
import { JwtModule } from '@nestjs/jwt';
import { ConfigModule, ConfigService } from '@nestjs/config';
import { LocalStrategy } from './local.strategy';
import { JwtStrategy } from './jwt.strategy';
import { AuthController } from './auth.controller';
import { PasswordService } from './password.service';
import { PasswordHistory } from './password-history.entity';
import { User } from '../users/user.entity';
import { AgreementModule } from '../agreement/agreement.module';

@Module({
  imports: [
    TypeOrmModule.forFeature([PasswordHistory, User]),
    UsersModule,
    PassportModule,
    ConfigModule,
    AgreementModule,
    JwtModule.registerAsync({
      imports: [ConfigModule],
      inject: [ConfigService],
      useFactory: async (configService: ConfigService) => ({
        secret: configService.get<string>('JWT_SECRET') || 'DEV_SECRET_KEY',
        signOptions: { expiresIn: '60m' },
      }),
    }),
  ],
  providers: [AuthService, LocalStrategy, JwtStrategy, PasswordService],
  controllers: [AuthController],
  exports: [AuthService, PasswordService],
})
export class AuthModule {}
