import {
  Controller,
  Get,
  Post,
  Patch,
  Delete,
  Body,
  Query,
  Param,
  UseGuards,
  ParseIntPipe,
} from '@nestjs/common';
import { JwtAuthGuard } from '../auth/jwt-auth.guard';
import { AdminGuard } from './admin.guard';
import { AdminService } from './admin.service';
import { DeviceBindingService } from '../devices/device-binding.service';
import { UsersService } from '../users/users.service';
import {
  ListUsersQueryDto,
  UpdateUserStatusDto,
  ResetPasswordDto,
  DeleteUserDto,
} from './dto/admin-users.dto';

@Controller('admin/users')
@UseGuards(JwtAuthGuard, AdminGuard)
export class AdminUsersController {
  constructor(
    private adminService: AdminService,
    private deviceBindingService: DeviceBindingService,
    private usersService: UsersService,
  ) {}

  @Get()
  async listUsers(@Query() query: ListUsersQueryDto) {
    return this.adminService.listUsers(query);
  }

  @Get(':id')
  async getUserDetail(@Param('id', ParseIntPipe) userId: number) {
    return this.adminService.getUserDetail(userId);
  }

  @Patch(':id/status')
  async updateUserStatus(
    @Param('id', ParseIntPipe) userId: number,
    @Body() dto: UpdateUserStatusDto,
  ) {
    await this.adminService.updateUserStatus(userId, dto);
    return { message: 'User status updated successfully' };
  }

  @Post(':id/reset-password')
  async resetUserPassword(
    @Param('id', ParseIntPipe) userId: number,
    @Body() dto: ResetPasswordDto,
  ) {
    return this.adminService.resetUserPassword(userId, dto);
  }

  @Delete(':id')
  async deleteUser(
    @Param('id', ParseIntPipe) userId: number,
    @Body() dto: DeleteUserDto,
  ) {
    await this.adminService.deleteUser(userId, dto);
    return { message: 'User deleted successfully' };
  }

  @Get(':id/devices')
  async getUserDevices(@Param('id', ParseIntPipe) userId: number) {
    return this.adminService.getUserDevices(userId);
  }
}
