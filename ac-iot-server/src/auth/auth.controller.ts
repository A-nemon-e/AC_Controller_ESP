import {
  Controller,
  Request,
  Post,
  UseGuards,
  Body,
  BadRequestException,
  Get,
} from '@nestjs/common';
import { LocalAuthGuard } from './local-auth.guard';
import { AuthService } from './auth.service';
import { RegisterDto } from './dto/register.dto';
import { AgreementService } from '../agreement/agreement.service';
import { AcceptAgreementDto } from '../agreement/dto/accept-agreement.dto';
import { JwtAuthGuard } from './jwt-auth.guard';

@Controller('auth')
export class AuthController {
  constructor(
    private authService: AuthService,
    private agreementService: AgreementService,
  ) {}

  @Post('register')
  async register(@Body() dto: RegisterDto) {
    // 检查是否有生效的协议
    const currentAgreement = await this.agreementService.getCurrentAgreement();

    if (currentAgreement) {
      // 如果有协议，必须接受协议才能注册
      if (!dto.agreementAccepted) {
        throw new BadRequestException('Must accept the user agreement to register');
      }

      if (!dto.agreementId || dto.agreementId !== currentAgreement.id) {
        throw new BadRequestException('Invalid agreement ID');
      }
    }

    return this.authService.register(dto.username, dto.password);
  }

  @Get('agreement/current')
  async getCurrentAgreement() {
    return this.agreementService.getCurrentAgreement();
  }

  @Post('agreement/accept')
  @UseGuards(JwtAuthGuard)
  async acceptAgreement(
    @Request() req: any,
    @Body() dto: AcceptAgreementDto,
  ) {
    await this.agreementService.acceptAgreement(
      req.user.userId,
      dto.agreementId,
      req.ip,
      req.headers['user-agent'] || '',
    );
    return { message: 'Agreement accepted successfully' };
  }

  @UseGuards(LocalAuthGuard)
  @Post('login')
  async login(@Request() req: any) {
    return this.authService.login(req.user);
  }
}
