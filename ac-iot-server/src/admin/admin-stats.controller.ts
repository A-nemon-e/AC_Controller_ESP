import { Controller, Get, UseGuards, Query } from '@nestjs/common';
import { JwtAuthGuard } from '../auth/jwt-auth.guard';
import { AdminGuard } from './admin.guard';
import { AdminService } from './admin.service';

@Controller('admin/stats')
@UseGuards(JwtAuthGuard, AdminGuard)
export class AdminStatsController {
  constructor(private adminService: AdminService) {}

  @Get('dashboard')
  async getDashboardStats() {
    return this.adminService.getDashboardStats();
  }

  @Get('users')
  async getUserStats(@Query('period') period: string = 'day') {
    return this.adminService.getUserStats(period);
  }

  @Get('devices')
  async getDeviceStats(@Query('period') period: string = 'day') {
    return this.adminService.getDeviceStats(period);
  }
}
