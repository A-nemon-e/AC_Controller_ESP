import {
  Injectable,
  NotFoundException,
  BadRequestException,
} from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository, MoreThanOrEqual } from 'typeorm';
import * as bcrypt from 'bcrypt';
import { User, UserRole } from '../users/user.entity';
import { Device } from '../devices/device.entity';
import {
  ListUsersQueryDto,
  UpdateUserStatusDto,
  ResetPasswordDto,
  DeleteUserDto,
} from './dto/admin-users.dto';
import {
  ListDevicesQueryDto,
  ForceUnbindDto,
  DebugCommandDto,
} from './dto/admin-devices.dto';

@Injectable()
export class AdminService {
  constructor(
    @InjectRepository(User)
    private userRepository: Repository<User>,
    @InjectRepository(Device)
    private deviceRepository: Repository<Device>,
  ) {}

  async getDashboardStats() {
    const today = new Date();
    today.setHours(0, 0, 0, 0);

    const [
      totalUsers,
      activeUsersToday,
      totalDevices,
      onlineDevices,
      newUsersToday,
    ] = await Promise.all([
      this.userRepository.count(),
      this.userRepository.count({
        where: { lastLoginAt: MoreThanOrEqual(today) },
      }),
      this.deviceRepository.count(),
      this.deviceRepository.count({ where: { isOnline: true } }),
      this.userRepository.count({
        where: { createdAt: MoreThanOrEqual(today) },
      }),
    ]);

    return {
      totalUsers,
      activeUsersToday,
      totalDevices,
      onlineDevices,
      newUsersToday,
      newDevicesToday: 0,
      apiCallsToday: 0,
      alerts: [],
    };
  }

  async getUserStats(period: string) {
    const total = await this.userRepository.count();
    const active = await this.userRepository.count({
      where: { isActive: true },
    });

    const byRole: Record<string, number> = {};
    for (const role of Object.values(UserRole)) {
      byRole[role] = await this.userRepository.count({ where: { role } });
    }

    return {
      total,
      active,
      inactive: total - active,
      byRole,
      registrationTrend: [],
    };
  }

  async getDeviceStats(period: string) {
    const total = await this.deviceRepository.count();
    const online = await this.deviceRepository.count({
      where: { isOnline: true },
    });

    return {
      total,
      online,
      offline: total - online,
      byBrand: {},
      byFirmware: {},
      connectionTrend: [],
    };
  }

  async listUsers(query: ListUsersQueryDto) {
    const qb = this.userRepository
      .createQueryBuilder('user')
      .leftJoinAndSelect('user.settings', 'settings')
      .select([
        'user.id',
        'user.username',
        'user.role',
        'user.isActive',
        'user.createdAt',
        'user.lastLoginAt',
      ]);

    if (query.search) {
      qb.andWhere('user.username LIKE :search', {
        search: `%${query.search}%`,
      });
    }

    if (query.role) {
      qb.andWhere('user.role = :role', { role: query.role });
    }

    if (query.isActive !== undefined) {
      qb.andWhere('user.isActive = :isActive', { isActive: query.isActive });
    }

    qb.orderBy(`user.${query.sortBy}`, query.sortOrder);

    const page = query.page ?? 1;
    const pageSize = query.pageSize ?? 20;

    const [users, total] = await qb
      .skip((page - 1) * pageSize)
      .take(pageSize)
      .getManyAndCount();

    const usersWithDeviceCount = await Promise.all(
      users.map(async (user) => {
        const deviceCount = await this.deviceRepository.count({
          where: { userId: user.id },
        });
        return {
          ...user,
          deviceCount,
        };
      }),
    );

    return {
      users: usersWithDeviceCount,
      total,
      page: page,
      pageSize: pageSize,
    };
  }

  async getUserDetail(userId: number) {
    const user = await this.userRepository.findOne({
      where: { id: userId },
      relations: ['settings'],
    });

    if (!user) {
      throw new NotFoundException('User not found');
    }

    const devices = await this.deviceRepository.find({
      where: { userId },
    });

    return {
      ...user,
      devices,
    };
  }

  async updateUserStatus(
    userId: number,
    dto: UpdateUserStatusDto,
  ): Promise<void> {
    const user = await this.userRepository.findOne({ where: { id: userId } });

    if (!user) {
      throw new NotFoundException('User not found');
    }

    user.isActive = dto.isActive;
    await this.userRepository.save(user);
  }

  async resetUserPassword(
    userId: number,
    dto: ResetPasswordDto,
  ): Promise<{ tempPassword: string }> {
    const user = await this.userRepository.findOne({ where: { id: userId } });

    if (!user) {
      throw new NotFoundException('User not found');
    }

    const tempPassword =
      dto.tempPassword || this.generateTempPassword();
    const hashedPassword = await bcrypt.hash(tempPassword, 10);

    user.password = hashedPassword;
    user.requirePasswordChange = dto.requireChange ?? true;
    user.passwordChangedAt = new Date();
    await this.userRepository.save(user);

    return { tempPassword };
  }

  private generateTempPassword(): string {
    const chars =
      'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    let password = '';
    for (let i = 0; i < 12; i++) {
      password += chars.charAt(Math.floor(Math.random() * chars.length));
    }
    return password;
  }

  async deleteUser(userId: number, dto: DeleteUserDto): Promise<void> {
    const user = await this.userRepository.findOne({ where: { id: userId } });

    if (!user) {
      throw new NotFoundException('User not found');
    }

    const devices = await this.deviceRepository.find({
      where: { userId },
    });

    if (devices.length > 0) {
      if (dto.transferDevices && dto.transferToUserId) {
        const targetUser = await this.userRepository.findOne({
          where: { id: dto.transferToUserId },
        });
        if (!targetUser) {
          throw new BadRequestException('Target user not found');
        }

        for (const device of devices) {
          device.userId = dto.transferToUserId;
          await this.deviceRepository.save(device);
        }
      } else {
        throw new BadRequestException(
          'User has devices. Either transfer them or delete them first.',
        );
      }
    }

    await this.userRepository.remove(user);
  }

  async getUserDevices(userId: number) {
    const user = await this.userRepository.findOne({ where: { id: userId } });

    if (!user) {
      throw new NotFoundException('User not found');
    }

    return this.deviceRepository.find({
      where: { userId },
    });
  }

  async listAllDevices(query: ListDevicesQueryDto) {
    const qb = this.deviceRepository
      .createQueryBuilder('device')
      .leftJoinAndSelect('device.user', 'user')
      .select([
        'device.id',
        'device.uuid',
        'device.name',
        'device.userId',
        'device.isOnline',
        'device.lastSeen',
        'device.bindStatus',
        'user.username',
      ]);

    if (query.search) {
      qb.andWhere(
        '(device.name LIKE :search OR device.uuid LIKE :search)',
        { search: `%${query.search}%` },
      );
    }

    if (query.status) {
      qb.andWhere('device.bindStatus = :status', { status: query.status });
    }

    qb.orderBy(`device.${query.sortBy}`, query.sortOrder);

    const page = query.page ?? 1;
    const pageSize = query.pageSize ?? 20;

    const [devices, total] = await qb
      .skip((page - 1) * pageSize)
      .take(pageSize)
      .getManyAndCount();

    const onlineCount = await this.deviceRepository.count({
      where: { isOnline: true },
    });

    return {
      devices,
      total,
      onlineCount,
      page: page,
      pageSize: pageSize,
    };
  }

  async getDeviceDetail(deviceId: number) {
    const device = await this.deviceRepository.findOne({
      where: { id: deviceId },
      relations: ['user'],
    });

    if (!device) {
      throw new NotFoundException('Device not found');
    }

    return device;
  }

  async forceUnbindDevice(
    deviceId: number,
    dto: ForceUnbindDto,
  ): Promise<void> {
    const device = await this.deviceRepository.findOne({
      where: { id: deviceId },
    });

    if (!device) {
      throw new NotFoundException('Device not found');
    }

    // Backup config if requested
    if (dto.backupConfig !== false) {
      device.backupConfig = {
        brandConfig: device.brandConfig,
        irConfig: device.irConfig,
        micConfig: device.micConfig,
        setupStatus: device.setupStatus,
        backedUpAt: new Date().toISOString(),
      };
      device.isRebindable = true;
    }

    (device as any).userId = null;
    device.bindStatus = 'unbound';
    device.lastUnbindTime = new Date();
    (device as any).boundUserId = null;

    await this.deviceRepository.save(device);
  }

  async getDeviceLogs(
    deviceId: number,
    query: { limit?: string; action?: string },
  ) {
    const device = await this.deviceRepository.findOne({
      where: { id: deviceId },
    });

    if (!device) {
      throw new NotFoundException('Device not found');
    }

    // Placeholder for device logs - in a real implementation,
    // this would query an audit log or device log repository
    return {
      deviceId,
      logs: [],
      message: 'Device logs feature not yet implemented',
    };
  }

  async sendDebugCommand(
    deviceId: number,
    dto: DebugCommandDto,
  ): Promise<void> {
    const device = await this.deviceRepository.findOne({
      where: { id: deviceId },
    });

    if (!device) {
      throw new NotFoundException('Device not found');
    }

    // Placeholder for sending debug commands via MQTT
    // In a real implementation, this would use MqttService
    console.log(
      `Debug command sent to device ${device.uuid}:`,
      dto.command,
      dto.payload,
    );
  }
}
