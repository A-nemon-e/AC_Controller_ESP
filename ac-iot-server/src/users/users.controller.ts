import { Controller, Get, Patch, Post, Body, UseGuards, Request } from '@nestjs/common';
import { UsersService } from './users.service';
import { JwtAuthGuard } from '../auth/jwt-auth.guard';
import { UpdateUserSettingsDto, ChangePasswordDto } from './dto/user-settings.dto';

@UseGuards(JwtAuthGuard)
@Controller('users')
export class UsersController {
    constructor(private readonly usersService: UsersService) { }

    @Get('me/settings')
    getSettings(@Request() req: any) {
        return this.usersService.getSettings(req.user.userId);
    }

    @Patch('me/settings')
    updateSettings(@Request() req: any, @Body() dto: UpdateUserSettingsDto) {
        return this.usersService.updateSettings(req.user.userId, dto);
    }

    @Post('me/change-password')
    changePassword(@Request() req: any, @Body() dto: ChangePasswordDto) {
        return this.usersService.changePassword(req.user.userId, dto);
    }
}
