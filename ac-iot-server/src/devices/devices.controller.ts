import { Controller, Get, Post, Body, Param, Delete, Patch, UseGuards, Request, ParseIntPipe, Query, NotFoundException, ForbiddenException } from '@nestjs/common';
import { DevicesService } from './devices.service';
import { CreateDeviceDto } from './dto/create-device.dto';
import { AcCommandDto } from './dto/command.dto';
import { UpdateDeviceConfigDto } from './dto/update-device-config.dto';
import { SetupBrandDto, SetupLearnAllDto } from './dto/setup.dto';
import { JwtAuthGuard } from '../auth/jwt-auth.guard';
import { LearnService } from './learn.service';
import { DeviceDiscoveryService } from './device-discovery.service';

@UseGuards(JwtAuthGuard)
@Controller('devices')
export class DevicesController {
    constructor(
        private readonly devicesService: DevicesService,
        private readonly learnService: LearnService,
        private readonly deviceDiscoveryService: DeviceDiscoveryService,
    ) { }

    @Post()
    create(@Request() req: any, @Body() createDeviceDto: CreateDeviceDto) {
        return this.devicesService.create(req.user.userId, createDeviceDto);
    }

    @Get()
    findAll(@Request() req: any) {
        return this.devicesService.findAll(req.user.userId);
    }

    // ===== 设备发现 API (静态路由优先) =====

    /**
     * 获取所有可发现的未绑定设备
     */
    @Get('discovery/available')
    async getAvailableDevices(@Query('maxAge') maxAge?: string) {
        console.log('[Controller] 收到 discovery/available 请求'); // 直接 console.log 确保能看到
        const maxAgeMinutes = maxAge ? parseInt(maxAge) : 5;
        return {
            devices: this.deviceDiscoveryService.getAvailableDevices(maxAgeMinutes),
            count: this.deviceDiscoveryService.getDiscoveredDeviceCount(),
        };
    }

    @Delete(':id')
    remove(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        return this.devicesService.remove(req.user.userId, id);
    }

    @Post(':id/cmd')
    sendCommand(
        @Request() req: any,
        @Param('id', ParseIntPipe) id: number,
        @Body() commandDto: AcCommandDto,
    ) {
        return this.devicesService.sendCommand(req.user.userId, id, commandDto);
    }

    @Get(':id/logs')
    getLogs(
        @Request() req: any,
        @Param('id', ParseIntPipe) id: number,
        @Query('action') action?: string,
        @Query('limit') limit?: string,
        @Query('since') since?: string,
    ) {
        return this.devicesService.getLogs(
            req.user.userId,
            id,
            limit ? parseInt(limit) : 50,
            action,
            since ? new Date(since) : undefined,
        );
    }

    @Get(':id/readings')
    getReadings(
        @Request() req: any,
        @Param('id', ParseIntPipe) id: number,
    ) {
        return this.devicesService.getReadings(req.user.userId, id);
    }

    @Get(':id/config')
    getConfig(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        return this.devicesService.getConfig(req.user.userId, id);
    }

    @Patch(':id/config')
    updateConfig(
        @Request() req: any,
        @Param('id', ParseIntPipe) id: number,
        @Body() dto: UpdateDeviceConfigDto,
    ) {
        return this.devicesService.updateConfig(req.user.userId, id, dto);
    }

    // ===== 红外学习 =====

    @Post(':id/learn/start')
    async startLearning(
        @Request() req: any,
        @Param('id', ParseIntPipe) id: number,
        @Body() body: { key: string },
    ) {
        const device = await this.devicesService.findOneById(id);
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== req.user.userId) throw new ForbiddenException('Access denied');

        return this.learnService.startLearning(req.user.userId, device.uuid, body.key);
    }

    @Get(':id/learn/status')
    async getLearningStatus(
        @Request() req: any,
        @Param('id', ParseIntPipe) id: number,
    ) {
        const device = await this.devicesService.findOneById(id);
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== req.user.userId) throw new ForbiddenException('Access denied');

        return this.learnService.getStatus(device.uuid);
    }

    @Delete(':id/learn/:key')
    async deleteLearnedCode(
        @Request() req: any,
        @Param('id', ParseIntPipe) id: number,
        @Param('key') key: string,
    ) {
        const device = await this.devicesService.findOneById(id);
        if (!device) throw new NotFoundException('Device not found');
        if (device.userId !== req.user.userId) throw new ForbiddenException('Access denied');

        if (device.irConfig && device.irConfig[key]) {
            delete device.irConfig[key];
            await this.devicesService.updateConfig(req.user.userId, id, { irConfig: device.irConfig });
        }

        return { message: 'Deleted' };
    }

    // ===== 设备初始化向导 =====

    @Get(':id/setup/status')
    getSetupStatus(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        return this.devicesService.getSetupStatus(req.user.userId, id);
    }

    @Post(':id/setup/brand')
    setBrand(
        @Request() req: any,
        @Param('id', ParseIntPipe) id: number,
        @Body() dto: SetupBrandDto,
    ) {
        return this.devicesService.setBrand(req.user.userId, id, dto.brandId, dto.model);
    }

    @Post(':id/setup/learn-all')
    startLearnAll(
        @Request() req: any,
        @Param('id', ParseIntPipe) id: number,
        @Body() dto: SetupLearnAllDto,
    ) {
        return this.devicesService.startLearnAll(req.user.userId, id, dto.keys);
    }

    @Post(':id/setup/complete')
    completeSetup(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        return this.devicesService.completeSetup(req.user.userId, id);
    }

    // ===== ✅ 品牌协议支持 =====

    @Get('brands')
    getSupportedBrands() {
        return {
            brands: [
                { id: 'GREE', name: '格力', models: [0, 1, 2] },
                { id: 'MIDEA', name: '美的', models: [0, 1] },
                { id: 'DAIKIN', name: '大金', models: [0, 1, 2, 3] },
                { id: 'HAIER', name: '海尔', models: [0, 1] },
                { id: 'MITSUBISHI', name: '三菱', models: [0, 1] },
                { id: 'PANASONIC', name: '松下', models: [0, 1] },
                { id: 'SAMSUNG', name: '三星', models: [0] },
                { id: 'LG', name: 'LG', models: [0, 1] },
                { id: 'FUJITSU', name: '富士通', models: [0, 1] },
                { id: 'TCL', name: 'TCL', models: [0] },
                { id: 'HISENSE', name: '海信', models: [0] },
                { id: 'COOLIX', name: '奥克斯', models: [0] },
                { id: 'TOSHIBA', name: '东芝', models: [0, 1] },
                { id: 'WHIRLPOOL', name: '惠而浦', models: [0] },
                { id: 'SHARP', name: '夏普', models: [0, 1] },
                { id: 'HITACHI', name: '日立', models: [0, 1, 2, 3] },
                { id: 'CARRIER', name: '开利', models: [0, 1] },
                { id: 'YORK', name: '约克', models: [0] },
            ]
        };
    }

    // ===== 自动协议检测 API =====

    /**
     * 启动自动协议检测
     */
    @Post(':id/auto-detect/start')
    async startAutoDetect(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        const device = await this.devicesService.findOneById(id);

        if (!device || device.userId !== req.user.userId) {
            throw new ForbiddenException('无权访问此设备');
        }

        // 发送MQTT命令到ESP
        return this.devicesService.startAutoDetect(req.user.userId, id);
    }

    /**
     * 停止自动协议检测
     */
    @Post(':id/auto-detect/stop')
    async stopAutoDetect(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        const device = await this.devicesService.findOneById(id);

        if (!device || device.userId !== req.user.userId) {
            throw new ForbiddenException('无权访问此设备');
        }

        return this.devicesService.stopAutoDetect(req.user.userId, id);
    }

    /**
     * 获取自动检测状态
     */
    @Get(':id/auto-detect/status')
    async getAutoDetectStatus(@Request() req: any, @Param('id', ParseIntPipe) id: number) {
        const device = await this.devicesService.findOneById(id);

        if (!device || device.userId !== req.user.userId) {
            throw new ForbiddenException('无权访问此设备');
        }

        return this.devicesService.getAutoDetectStatus(id);
    }

    // ===== 设备发现 API =====

    /**
     * 手动清理离线设备
     */
    @Post('discovery/cleanup')
    cleanupOfflineDevices() {
        const count = this.deviceDiscoveryService.cleanupOfflineDevices(10);
        return {
            message: `已清理 ${count} 个离线设备`,
            cleanedCount: count,
        };
    }
}
