import {
  Controller,
  Get,
  Post,
  Body,
  UseGuards,
  Req,
  Request,
} from '@nestjs/common';
import { AgreementService } from './agreement.service';
import { JwtAuthGuard } from '../auth/jwt-auth.guard';
import { AcceptAgreementDto } from './dto/accept-agreement.dto';

@Controller('agreement')
export class AgreementController {
  constructor(private agreementService: AgreementService) {}

  @Get('current')
  async getCurrentAgreement() {
    return this.agreementService.getCurrentAgreement();
  }

  @Post('accept')
  @UseGuards(JwtAuthGuard)
  async acceptAgreement(
    @Request() req: any,
    @Body() dto: AcceptAgreementDto,
    @Req() request: any,
  ): Promise<{ success: boolean }> {
    const userId = req.user.userId;

    await this.agreementService.acceptAgreement(
      userId,
      dto.agreementId,
      request.ip,
      request.headers['user-agent'],
    );

    return { success: true };
  }
}
