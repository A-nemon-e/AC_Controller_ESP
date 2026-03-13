import {
  Controller,
  Get,
  Post,
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
import {
  ListDevicesQueryDto,
  ForceUnbindDto,
  DebugCommandDto,
} from './dto/admin-devices.dto';

@Controller('admin/devices')
@UseGuards(JwtAuthGuard, AdminGuard)
export class AdminDevicesController {
  constructor(
    private adminService: AdminService,
    private deviceBindingService: DeviceBindingService,
  ) {}

  @Get()
  async listAllDevices(@Query() query: ListDevicesQueryDto) {
    return this.adminService.listAllDevices(query);
  }

  @Get(':id')
  async getDeviceDetail(@Param('id', ParseIntPipe) deviceId: number) {
    return this.adminService.getDeviceDetail(deviceId);
  }

  @Post(':id/unbind')
  async forceUnbindDevice(
    @Param('id', ParseIntPipe) deviceId: number,
    @Body() dto: ForceUnbindDto,
  ) {
    await this.adminService.forceUnbindDevice(deviceId, dto);
    return { message: 'Device unbound successfully' };
  }

  @Get(':id/logs')
  async getDeviceLogs(
    @Param('id', ParseIntPipe) deviceId: number,
    @Query() query: { limit?: string; action?: string },
  ) {
    return this.adminService.getDeviceLogs(deviceId, query);
  }

  @Post(':id/debug')
  async sendDebugCommand(
    @Param('id', ParseIntPipe) deviceId: number,
    @Body() dto: DebugCommandDto,
  ) {
    await this.adminService.sendDebugCommand(deviceId, dto);
    return { message: 'Debug command sent successfully' };
  }
}
