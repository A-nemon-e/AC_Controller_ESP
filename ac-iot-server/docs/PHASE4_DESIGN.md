# Phase 4: 后端改进 - 详细设计文档

> **阶段**: @plan - Phase 4  
> **日期**: 2026-03-12  
> **前置依赖**: 无（可并行于Phase 1-3）  
> **状态**: 规划中
> **实施进度**: 0% (待开始)

---

## 1. 目标

改进后端服务的健壮性和功能性，包括设备绑定逻辑、用户系统扩展和管理后台API。

**成功标准**:
- [x] 设备绑定逻辑健壮性改进（防重复绑定、防意外掉绑定）(80% - device-binding.service.ts实现)
- [x] 用户注册添加协议确认 (100% - agreement模块完整)
- [x] 实现用户修改密码功能 (100% - auth模块完整)
- [x] 实现管理后台API（用户管理、设备监控、系统统计）(70% - admin.service.ts基础实现)
- [ ] 所有API通过单元测试 (0% - 需要补充测试)
- [ ] 安全性审计通过 (0% - 需要审计)

**实施进度**: 75% (核心功能实现，待测试和审计)

---

## 2. 系统架构

### 2.1 改进范围

```
┌─────────────────────────────────────────────────────────────────┐
│                      Phase 4 改进范围                            │
└─────────────────────────────────────────────────────────────────┘

现有后端服务:
┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│   Auth       │  │   Users      │  │   Devices    │  │   Routines   │
│   Module     │  │   Module     │  │   Module     │  │   Module     │
└──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘
       │                 │                 │                 │
       ▼                 ▼                 ▼                 ▼
  ┌──────────┐      ┌──────────┐      ┌──────────┐      ┌──────────┐
  │+用户协议 │      │+改密码   │      │+绑定健壮 │      │ 保持     │
  │+协议版本 │      │+安全检查│      │+绑定恢复 │      │ 不变     │
  └──────────┘      └──────────┘      └──────────┘      └──────────┘
                                           │
                                           ▼
                                    ┌──────────────┐
                                    │  Admin       │
                                    │  Module(新增)│
                                    ├──────────────┤
                                    │+用户管理API  │
                                    │+设备监控API  │
                                    │+系统统计API  │
                                    │+OTA管理API   │
                                    └──────────────┘
```

---

## 3. 模块1: 设备绑定逻辑改进

### 3.1 当前问题分析

根据CODE_ANALYSIS.md的分析，现有绑定逻辑存在以下问题：

1. **内存缓存无持久化**: `autoDetectStates` 和 `learningStates` 使用Map，服务重启丢失
2. **绑定状态不一致**: ESP端和服务器端可能出现状态不同步
3. **缺少防重复绑定机制**: 同一设备可能被多个用户重复绑定
4. **解绑后无法恢复**: 用户误操作解绑后，设备配置丢失

### 3.2 改进方案

#### 3.2.1 绑定状态机

```
┌─────────────────────────────────────────────────────────────┐
│                    设备绑定状态机                            │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│    ┌──────────┐                                             │
│    │  UNBOUND │◄───────────────────────────────────┐        │
│    │ (未绑定) │                                    │        │
│    └────┬─────┘                                    │        │
│         │ 1. 设备首次连接                           │        │
│         │    发送discovery消息                      │        │
│         ▼                                          │        │
│    ┌──────────┐     2. 用户绑定设备                 │        │
│    │BINDING   │◄────────────────────────────┐      │        │
│    │(绑定中)  │                             │      │        │
│    └────┬─────┘                             │      │        │
│         │ 3. 推送配置                         │      │        │
│         │    MQTT config/update              │      │        │
│         ▼                                    │      │        │
│    ┌──────────┐     4. 配置确认               │      │        │
│    │  BOUND   │─────────────────────────────┘      │        │
│    │ (已绑定) │                                    │        │
│    └────┬─────┘                                    │        │
│         │ 5. 用户解绑/设备重置                      │        │
│         │                                          │        │
│         ▼                                          │        │
│    ┌──────────┐     6. 恢复绑定（保留数据）        │        │
│    │ UNBOUND  │────────────────────────────────────┘        │
│    │ (保留配置)│                                            │
│    └──────────┘                                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

#### 3.2.2 数据库实体改进

**新增 DeviceBinding 实体（绑定历史）**

```typescript
@Entity()
export class DeviceBinding {
    @PrimaryGeneratedColumn()
    id: number;

    @Column()
    deviceId: number;

    @Column()
    userId: number;

    @Column({ type: 'enum', enum: ['bind', 'unbind', 'rebind'] })
    action: string;

    @Column({ type: 'text', nullable: true })
    reason: string;  // 解绑原因

    @CreateDateColumn()
    timestamp: Date;

    @Index(['deviceId', 'timestamp'])
    deviceTimestampIdx: Date;
}
```

**改进 Device 实体**

```typescript
@Entity()
export class Device {
    // 现有字段...
    
    // 新增字段
    @Column({ default: 'pending' })
    bindStatus: 'pending' | 'bound' | 'unbound' | 'error';

    @Column({ type: 'datetime', nullable: true })
    lastBindTime: Date;

    @Column({ type: 'datetime', nullable: true })
    lastUnbindTime: Date;

    @Column({ nullable: true })
    boundUserId: number;

    @Column({ default: false })
    isRebindable: boolean;  // 是否允许重新绑定（保留配置）

    @Column({ type: 'simple-json', nullable: true })
    backupConfig: any;  // 解绑时备份的配置
}
```

#### 3.2.3 绑定服务实现

**DeviceBindingService**

```typescript
@Injectable()
export class DeviceBindingService {
    private readonly logger = new Logger(DeviceBindingService.name);

    constructor(
        @InjectRepository(Device)
        private deviceRepository: Repository<Device>,
        @InjectRepository(DeviceBinding)
        private bindingRepository: Repository<DeviceBinding>,
        private mqttService: MqttService,
    ) {}

    /**
     * 创建设备绑定（改进版）
     */
    async bindDevice(
        userId: number, 
        createDeviceDto: CreateDeviceDto
    ): Promise<Device> {
        const { uuid, mac } = createDeviceDto;

        // 1. 检查是否已绑定（通过UUID）
        let device = await this.deviceRepository.findOne({
            where: { uuid },
            relations: ['user']
        });

        if (device) {
            // 2. 防重复绑定检查
            if (device.userId === userId && device.bindStatus === 'bound') {
                this.logger.warn(`Device ${uuid} already bound to current user`);
                throw new ConflictException('Device already bound to your account');
            }

            // 3. 检查是否被其他用户绑定
            if (device.userId && device.userId !== userId) {
                // 检查是否可重新绑定
                if (!device.isRebindable) {
                    this.logger.warn(`Device ${uuid} bound to another user`);
                    throw new ForbiddenException('Device bound to another user');
                }

                // 记录解绑日志
                await this.recordBinding(device.id, device.userId, 'unbind', 'reclaimed_by_another_user');
                this.logger.log(`Device ${uuid} reclaimed from user ${device.userId} to ${userId}`);
            }

            // 4. 恢复绑定（如果有备份配置）
            if (device.backupConfig) {
                this.logger.log(`Restoring config for device ${uuid}`);
                device.brandConfig = device.backupConfig.brandConfig || device.brandConfig;
                device.irConfig = device.backupConfig.irConfig || device.irConfig;
            }

            // 更新绑定信息
            device.userId = userId;
            device.bindStatus = 'bound';
            device.lastBindTime = new Date();
            device.boundUserId = userId;
        } else {
            // 5. 新设备创建
            device = this.deviceRepository.create({
                ...createDeviceDto,
                userId,
                bindStatus: 'bound',
                lastBindTime: new Date(),
                boundUserId: userId,
            });
        }

        // 6. 保存设备
        const saved = await this.deviceRepository.save(device);

        // 7. 记录绑定日志
        await this.recordBinding(saved.id, userId, 'bind', 'initial_binding');

        // 8. 推送配置到ESP
        await this.pushBindingConfig(saved);

        return saved;
    }

    /**
     * 解绑设备（改进版）
     */
    async unbindDevice(
        userId: number, 
        deviceId: number,
        options: { backupConfig?: boolean; reason?: string } = {}
    ): Promise<void> {
        const device = await this.deviceRepository.findOne({
            where: { id: deviceId }
        });

        if (!device) {
            throw new NotFoundException('Device not found');
        }

        if (device.userId !== userId) {
            throw new ForbiddenException('Not your device');
        }

        // 1. 备份配置（可选）
        if (options.backupConfig !== false) {
            device.backupConfig = {
                brandConfig: device.brandConfig,
                irConfig: device.irConfig,
                micConfig: device.micConfig,
                setupStatus: device.setupStatus,
                backedUpAt: new Date().toISOString()
            };
            device.isRebindable = true;
            this.logger.log(`Config backed up for device ${device.uuid}`);
        }

        // 2. 更新状态
        device.userId = null;
        device.bindStatus = 'unbound';
        device.lastUnbindTime = new Date();
        device.boundUserId = null;

        await this.deviceRepository.save(device);

        // 3. 记录解绑日志
        await this.recordBinding(deviceId, userId, 'unbind', options.reason || 'user_initiated');

        // 4. 推送解绑配置到ESP
        await this.pushUnbindConfig(device);

        this.logger.log(`Device ${device.uuid} unbound by user ${userId}`);
    }

    /**
     * 恢复绑定
     */
    async rebindDevice(userId: number, deviceId: number): Promise<Device> {
        const device = await this.deviceRepository.findOne({
            where: { id: deviceId }
        });

        if (!device) {
            throw new NotFoundException('Device not found');
        }

        if (!device.isRebindable || !device.backupConfig) {
            throw new BadRequestException('Device not eligible for rebind');
        }

        // 恢复绑定
        device.userId = userId;
        device.bindStatus = 'bound';
        device.lastBindTime = new Date();
        device.boundUserId = userId;

        const saved = await this.deviceRepository.save(device);

        // 记录日志
        await this.recordBinding(deviceId, userId, 'rebind', 'config_restored');

        this.logger.log(`Device ${device.uuid} rebound by user ${userId}`);

        return saved;
    }

    /**
     * 获取绑定历史
     */
    async getBindingHistory(deviceId: number): Promise<DeviceBinding[]> {
        return this.bindingRepository.find({
            where: { deviceId },
            order: { timestamp: 'DESC' },
            take: 50
        });
    }

    /**
     * 记录绑定操作
     */
    private async recordBinding(
        deviceId: number, 
        userId: number, 
        action: string,
        reason: string
    ): Promise<void> {
        await this.bindingRepository.save({
            deviceId,
            userId,
            action,
            reason,
            timestamp: new Date()
        });
    }

    /**
     * 推送绑定配置到ESP
     */
    private async pushBindingConfig(device: Device): Promise<void> {
        const topic = `ac/config/${device.mac.replace(/:/g, '')}`;
        const payload = {
            userId: device.userId,
            deviceId: device.id,
            bindStatus: 'bound',
            timestamp: Date.now()
        };

        await this.mqttService.publish(topic, JSON.stringify(payload));
    }

    /**
     * 推送解绑配置到ESP
     */
    private async pushUnbindConfig(device: Device): Promise<void> {
        const topic = `ac/user_${device.boundUserId}/dev_${device.uuid}/config/update`;
        const payload = {
            userId: 0,
            deviceId: 0,
            bindStatus: 'unbound',
            action: 'reset',
            timestamp: Date.now()
        };

        await this.mqttService.publish(topic, JSON.stringify(payload));
    }
}
```

#### 3.2.4 防掉绑定机制

**心跳检测**

```typescript
@Injectable()
export class DeviceHeartbeatService {
    private readonly logger = new Logger(DeviceHeartbeatService.name);
    private deviceLastSeen: Map<number, number> = new Map(); // deviceId -> timestamp

    constructor(
        @InjectRepository(Device)
        private deviceRepository: Repository<Device>,
        private eventEmitter: EventEmitter2,
    ) {}

    /**
     * 设备心跳上报
     */
    onDeviceHeartbeat(deviceId: number): void {
        this.deviceLastSeen.set(deviceId, Date.now());
    }

    /**
     * 检查离线设备
     */
    @Cron(CronExpression.EVERY_5_MINUTES)
    async checkOfflineDevices(): Promise<void> {
        const now = Date.now();
        const offlineThreshold = 5 * 60 * 1000; // 5分钟

        for (const [deviceId, lastSeen] of this.deviceLastSeen.entries()) {
            if (now - lastSeen > offlineThreshold) {
                await this.markDeviceOffline(deviceId);
            }
        }
    }

    private async markDeviceOffline(deviceId: number): Promise<void> {
        const device = await this.deviceRepository.findOne({
            where: { id: deviceId }
        });

        if (device && device.isOnline) {
            device.isOnline = false;
            await this.deviceRepository.save(device);
            
            this.eventEmitter.emit('device.offline', { deviceId });
            this.logger.log(`Device ${device.uuid} marked as offline`);
        }
    }
}
```

---

## 4. 模块2: 用户系统扩展

### 4.1 用户协议系统

#### 4.1.1 数据库实体

**UserAgreement（用户协议）**

```typescript
@Entity()
export class UserAgreement {
    @PrimaryGeneratedColumn()
    id: number;

    @Column()
    version: string;  // 协议版本，如 "v1.0"

    @Column({ type: 'text' })
    content: string;  // 协议内容（Markdown）

    @Column({ type: 'boolean', default: false })
    isCurrent: boolean;  // 是否当前生效版本

    @Column({ type: 'simple-json', nullable: true })
    changes: string[];  // 变更说明

    @CreateDateColumn()
    createdAt: Date;

    @Column({ type: 'datetime', nullable: true })
    effectiveDate: Date;  // 生效日期
}
```

**UserAgreementAcceptance（用户协议接受记录）**

```typescript
@Entity()
export class UserAgreementAcceptance {
    @PrimaryGeneratedColumn()
    id: number;

    @Column()
    userId: number;

    @Column()
    agreementId: number;

    @Column({ type: 'varchar', length: 45 })
    ipAddress: string;

    @Column()
    userAgent: string;

    @CreateDateColumn()
    acceptedAt: Date;

    @Index(['userId', 'agreementId'])
    userAgreementIdx: number;
}
```

#### 4.1.2 协议服务实现

```typescript
@Injectable()
export class AgreementService {
    constructor(
        @InjectRepository(UserAgreement)
        private agreementRepository: Repository<UserAgreement>,
        @InjectRepository(UserAgreementAcceptance)
        private acceptanceRepository: Repository<UserAgreementAcceptance>,
    ) {}

    /**
     * 获取当前生效的协议
     */
    async getCurrentAgreement(): Promise<UserAgreement> {
        return this.agreementRepository.findOne({
            where: { isCurrent: true },
            order: { effectiveDate: 'DESC' }
        });
    }

    /**
     * 检查用户是否已接受当前协议
     */
    async hasUserAccepted(userId: number): Promise<boolean> {
        const currentAgreement = await this.getCurrentAgreement();
        if (!currentAgreement) return true;  // 无协议要求

        const acceptance = await this.acceptanceRepository.findOne({
            where: {
                userId,
                agreementId: currentAgreement.id
            }
        });

        return !!acceptance;
    }

    /**
     * 用户接受协议
     */
    async acceptAgreement(
        userId: number, 
        agreementId: number,
        ipAddress: string,
        userAgent: string
    ): Promise<void> {
        await this.acceptanceRepository.save({
            userId,
            agreementId,
            ipAddress,
            userAgent,
            acceptedAt: new Date()
        });
    }

    /**
     * 发布新版本协议
     */
    async publishAgreement(
        version: string,
        content: string,
        changes: string[],
        effectiveDate?: Date
    ): Promise<UserAgreement> {
        // 将旧版本标记为非当前
        await this.agreementRepository.update(
            { isCurrent: true },
            { isCurrent: false }
        );

        // 创建新版本
        return this.agreementRepository.save({
            version,
            content,
            changes,
            isCurrent: true,
            effectiveDate: effectiveDate || new Date()
        });
    }
}
```

#### 4.1.3 注册流程改进

**AuthController 改进**

```typescript
@Controller('auth')
export class AuthController {
    @Post('register')
    async register(
        @Body() dto: RegisterDto,
        @Req() req
    ): Promise<{ access_token: string; requiresAgreement: boolean }> {
        // 1. 创建用户
        const user = await this.authService.register(dto);

        // 2. 检查是否需要接受协议
        const requiresAgreement = !(await this.agreementService.hasUserAccepted(user.id));

        // 3. 如果不需要协议或已接受，直接登录
        if (!requiresAgreement) {
            const token = await this.authService.login(user);
            return { access_token: token, requiresAgreement: false };
        }

        // 4. 需要接受协议，返回临时token
        const tempToken = await this.authService.generateTempToken(user);
        return { access_token: tempToken, requiresAgreement: true };
    }

    @Post('agreement/accept')
    @UseGuards(JwtAuthGuard)
    async acceptAgreement(
        @Request() req,
        @Body() dto: AcceptAgreementDto,
        @Req() request
    ): Promise<{ access_token: string }> {
        const userId = req.user.userId;
        
        // 记录接受
        await this.agreementService.acceptAgreement(
            userId,
            dto.agreementId,
            request.ip,
            request.headers['user-agent']
        );

        // 生成正式token
        const token = await this.authService.generateToken(userId);
        return { access_token: token };
    }

    @Get('agreement/current')
    async getCurrentAgreement(): Promise<UserAgreement> {
        return this.agreementService.getCurrentAgreement();
    }
}
```

**RegisterDto**

```typescript
export class RegisterDto {
    @IsString()
    @MinLength(3)
    @MaxLength(20)
    username: string;

    @IsString()
    @MinLength(6)
    password: string;

    @IsOptional()
    @IsEmail()
    email?: string;

    @IsBoolean()
    @IsOptional()
    agreementAccepted?: boolean;  // 是否已接受协议

    @IsNumber()
    @IsOptional()
    agreementId?: number;  // 接受的协议ID
}
```

### 4.2 修改密码功能

#### 4.2.1 密码安全策略

```typescript
// constants/security.ts
export const PASSWORD_POLICY = {
    minLength: 8,
    maxLength: 32,
    requireUppercase: true,
    requireLowercase: true,
    requireNumbers: true,
    requireSpecialChars: false,
    maxAge: 90,  // 密码最大有效期（天）
    historyCount: 3,  // 不能重复使用最近3次密码
};

export const LOGIN_SECURITY = {
    maxFailedAttempts: 5,
    lockoutDuration: 15,  // 锁定时间（分钟）
    requireCaptchaAfter: 3,  // 3次失败后需要验证码
};
```

#### 4.2.2 密码历史记录

```typescript
@Entity()
export class PasswordHistory {
    @PrimaryGeneratedColumn()
    id: number;

    @Column()
    userId: number;

    @Column()
    passwordHash: string;

    @CreateDateColumn()
    createdAt: Date;

    @Index(['userId', 'createdAt'])
    userCreatedIdx: Date;
}
```

#### 4.2.3 修改密码服务

```typescript
@Injectable()
export class PasswordService {
    constructor(
        @InjectRepository(User)
        private userRepository: Repository<User>,
        @InjectRepository(PasswordHistory)
        private passwordHistoryRepository: Repository<PasswordHistory>,
    ) {}

    /**
     * 修改密码
     */
    async changePassword(
        userId: number,
        currentPassword: string,
        newPassword: string
    ): Promise<void> {
        const user = await this.userRepository.findOne({
            where: { id: userId }
        });

        if (!user) {
            throw new NotFoundException('User not found');
        }

        // 1. 验证当前密码
        const isCurrentValid = await bcrypt.compare(currentPassword, user.password);
        if (!isCurrentValid) {
            throw new UnauthorizedException('Current password is incorrect');
        }

        // 2. 验证新密码策略
        await this.validatePasswordPolicy(newPassword);

        // 3. 检查是否重复使用历史密码
        await this.checkPasswordHistory(userId, newPassword);

        // 4. 更新密码
        const newHash = await bcrypt.hash(newPassword, 10);
        user.password = newHash;
        user.passwordChangedAt = new Date();
        await this.userRepository.save(user);

        // 5. 记录密码历史
        await this.passwordHistoryRepository.save({
            userId,
            passwordHash: newHash,
            createdAt: new Date()
        });

        // 6. 清理旧历史（只保留最近N个）
        await this.cleanupPasswordHistory(userId);
    }

    /**
     * 验证密码策略
     */
    private async validatePasswordPolicy(password: string): Promise<void> {
        const errors: string[] = [];

        if (password.length < PASSWORD_POLICY.minLength) {
            errors.push(`Password must be at least ${PASSWORD_POLICY.minLength} characters`);
        }

        if (password.length > PASSWORD_POLICY.maxLength) {
            errors.push(`Password must not exceed ${PASSWORD_POLICY.maxLength} characters`);
        }

        if (PASSWORD_POLICY.requireUppercase && !/[A-Z]/.test(password)) {
            errors.push('Password must contain at least one uppercase letter');
        }

        if (PASSWORD_POLICY.requireLowercase && !/[a-z]/.test(password)) {
            errors.push('Password must contain at least one lowercase letter');
        }

        if (PASSWORD_POLICY.requireNumbers && !/[0-9]/.test(password)) {
            errors.push('Password must contain at least one number');
        }

        if (PASSWORD_POLICY.requireSpecialChars && !/[!@#$%^&*]/.test(password)) {
            errors.push('Password must contain at least one special character');
        }

        if (errors.length > 0) {
            throw new BadRequestException(errors.join('; '));
        }
    }

    /**
     * 检查密码历史
     */
    private async checkPasswordHistory(
        userId: number, 
        newPassword: string
    ): Promise<void> {
        const history = await this.passwordHistoryRepository.find({
            where: { userId },
            order: { createdAt: 'DESC' },
            take: PASSWORD_POLICY.historyCount
        });

        for (const record of history) {
            const isMatch = await bcrypt.compare(newPassword, record.passwordHash);
            if (isMatch) {
                throw new BadRequestException(
                    `Cannot reuse the last ${PASSWORD_POLICY.historyCount} passwords`
                );
            }
        }
    }

    /**
     * 清理密码历史
     */
    private async cleanupPasswordHistory(userId: number): Promise<void> {
        const history = await this.passwordHistoryRepository.find({
            where: { userId },
            order: { createdAt: 'DESC' },
            skip: PASSWORD_POLICY.historyCount
        });

        if (history.length > 0) {
            await this.passwordHistoryRepository.remove(history);
        }
    }

    /**
     * 检查密码是否过期
     */
    async isPasswordExpired(userId: number): Promise<boolean> {
        const user = await this.userRepository.findOne({
            where: { id: userId },
            select: ['passwordChangedAt']
        });

        if (!user || !user.passwordChangedAt) {
            return false;
        }

        const daysSinceChange = Math.floor(
            (Date.now() - user.passwordChangedAt.getTime()) / (1000 * 60 * 60 * 24)
        );

        return daysSinceChange > PASSWORD_POLICY.maxAge;
    }
}
```

#### 4.2.4 用户控制器扩展

```typescript
@Controller('users')
@UseGuards(JwtAuthGuard)
export class UsersController {
    constructor(
        private passwordService: PasswordService,
        private userService: UsersService,
    ) {}

    @Post('me/change-password')
    async changePassword(
        @Request() req,
        @Body() dto: ChangePasswordDto
    ): Promise<void> {
        await this.passwordService.changePassword(
            req.user.userId,
            dto.currentPassword,
            dto.newPassword
        );
    }

    @Get('me/password-status')
    async getPasswordStatus(@Request() req): Promise<{
        expired: boolean;
        daysUntilExpiry?: number;
        lastChanged: Date;
    }> {
        const userId = req.user.userId;
        const expired = await this.passwordService.isPasswordExpired(userId);
        const user = await this.userService.findById(userId);

        let daysUntilExpiry: number | undefined;
        if (!expired && user.passwordChangedAt) {
            const daysSinceChange = Math.floor(
                (Date.now() - user.passwordChangedAt.getTime()) / (1000 * 60 * 60 * 24)
            );
            daysUntilExpiry = PASSWORD_POLICY.maxAge - daysSinceChange;
        }

        return {
            expired,
            daysUntilExpiry,
            lastChanged: user.passwordChangedAt
        };
    }
}
```

---

## 5. 模块3: 管理后台API

### 5.1 权限系统设计

#### 5.1.1 角色定义

```typescript
export enum UserRole {
    USER = 'user',           // 普通用户
    ADMIN = 'admin',         // 管理员
    SUPER_ADMIN = 'super_admin'  // 超级管理员
}

// 权限矩阵
export const PERMISSIONS = {
    // 用户管理
    'user.view': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
    'user.create': [UserRole.SUPER_ADMIN],
    'user.edit': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
    'user.delete': [UserRole.SUPER_ADMIN],
    'user.reset_password': [UserRole.ADMIN, UserRole.SUPER_ADMIN],

    // 设备管理
    'device.view_all': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
    'device.force_unbind': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
    'device.debug': [UserRole.SUPER_ADMIN],

    // 系统管理
    'system.config': [UserRole.SUPER_ADMIN],
    'system.logs': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
    'system.stats': [UserRole.ADMIN, UserRole.SUPER_ADMIN],

    // OTA管理
    'ota.upload': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
    'ota.push': [UserRole.ADMIN, UserRole.SUPER_ADMIN],
};
```

#### 5.1.2 管理员守卫

```typescript
@Injectable()
export class AdminGuard implements CanActivate {
    canActivate(context: ExecutionContext): boolean {
        const request = context.switchToHttp().getRequest();
        const user = request.user;

        if (!user) {
            throw new UnauthorizedException();
        }

        if (user.role === UserRole.USER) {
            throw new ForbiddenException('Admin access required');
        }

        return true;
    }
}

@Injectable()
export class PermissionGuard implements CanActivate {
    constructor(private requiredPermission: string) {}

    canActivate(context: ExecutionContext): boolean {
        const request = context.switchToHttp().getRequest();
        const user = request.user;

        const allowedRoles = PERMISSIONS[this.requiredPermission] || [];
        
        if (!allowedRoles.includes(user.role)) {
            throw new ForbiddenException(`Permission '${this.requiredPermission}' required`);
        }

        return true;
    }
}
```

### 5.2 用户管理API

#### 5.2.1 控制器实现

```typescript
@Controller('admin/users')
@UseGuards(JwtAuthGuard, AdminGuard)
export class AdminUsersController {
    constructor(
        private adminService: AdminService,
        private userService: UsersService,
    ) {}

    @Get()
    @UseGuards(new PermissionGuard('user.view'))
    async listUsers(
        @Query() query: ListUsersQueryDto
    ): Promise<{
        users: AdminUserDto[];
        total: number;
        page: number;
        pageSize: number;
    }> {
        return this.adminService.listUsers(query);
    }

    @Get(':id')
    @UseGuards(new PermissionGuard('user.view'))
    async getUserDetail(
        @Param('id', ParseIntPipe) userId: number
    ): Promise<AdminUserDetailDto> {
        return this.adminService.getUserDetail(userId);
    }

    @Patch(':id/status')
    @UseGuards(new PermissionGuard('user.edit'))
    async updateUserStatus(
        @Param('id', ParseIntPipe) userId: number,
        @Body() dto: UpdateUserStatusDto
    ): Promise<void> {
        await this.adminService.updateUserStatus(userId, dto);
    }

    @Post(':id/reset-password')
    @UseGuards(new PermissionGuard('user.reset_password'))
    async resetUserPassword(
        @Param('id', ParseIntPipe) userId: number,
        @Body() dto: ResetPasswordDto
    ): Promise<{ tempPassword: string }> {
        return this.adminService.resetUserPassword(userId, dto);
    }

    @Delete(':id')
    @UseGuards(new PermissionGuard('user.delete'))
    async deleteUser(
        @Param('id', ParseIntPipe) userId: number,
        @Body() dto: DeleteUserDto
    ): Promise<void> {
        await this.adminService.deleteUser(userId, dto);
    }

    @Get(':id/devices')
    @UseGuards(new PermissionGuard('user.view'))
    async getUserDevices(
        @Param('id', ParseIntPipe) userId: number
    ): Promise<AdminDeviceDto[]> {
        return this.adminService.getUserDevices(userId);
    }
}
```

#### 5.2.2 DTO定义

```typescript
export class ListUsersQueryDto {
    @IsOptional()
    @IsString()
    search?: string;

    @IsOptional()
    @IsEnum(UserRole)
    role?: UserRole;

    @IsOptional()
    @IsBoolean()
    isActive?: boolean;

    @IsOptional()
    @IsNumber()
    @Min(1)
    page?: number = 1;

    @IsOptional()
    @IsNumber()
    @Min(1)
    @Max(100)
    pageSize?: number = 20;

    @IsOptional()
    @IsIn(['createdAt', 'username', 'lastLogin'])
    sortBy?: string = 'createdAt';

    @IsOptional()
    @IsIn(['ASC', 'DESC'])
    sortOrder?: 'ASC' | 'DESC' = 'DESC';
}

export class AdminUserDto {
    id: number;
    username: string;
    email: string;
    role: UserRole;
    isActive: boolean;
    deviceCount: number;
    createdAt: Date;
    lastLoginAt: Date;
}

export class AdminUserDetailDto extends AdminUserDto {
    lastLoginIp: string;
    agreementsAccepted: string[];
    devices: AdminDeviceDto[];
    recentActivity: ActivityLogDto[];
}

export class UpdateUserStatusDto {
    @IsBoolean()
    isActive: boolean;

    @IsOptional()
    @IsString()
    reason?: string;
}

export class ResetPasswordDto {
    @IsOptional()
    @IsBoolean()
    requireChange?: boolean;  // 是否要求用户下次登录时修改密码

    @IsOptional()
    @IsString()
    tempPassword?: string;  // 如果不提供则自动生成
}

export class DeleteUserDto {
    @IsBoolean()
    transferDevices: boolean;  // 是否转移设备给其他管理员

    @IsOptional()
    @IsNumber()
    transferToUserId?: number;

    @IsString()
    reason: string;
}
```

### 5.3 设备管理API

```typescript
@Controller('admin/devices')
@UseGuards(JwtAuthGuard, AdminGuard)
export class AdminDevicesController {
    constructor(private adminService: AdminService) {}

    @Get()
    @UseGuards(new PermissionGuard('device.view_all'))
    async listAllDevices(
        @Query() query: ListDevicesQueryDto
    ): Promise<{
        devices: AdminDeviceDto[];
        total: number;
        onlineCount: number;
    }> {
        return this.adminService.listAllDevices(query);
    }

    @Get(':id')
    @UseGuards(new PermissionGuard('device.view_all'))
    async getDeviceDetail(
        @Param('id', ParseIntPipe) deviceId: number
    ): Promise<AdminDeviceDetailDto> {
        return this.adminService.getDeviceDetail(deviceId);
    }

    @Post(':id/unbind')
    @UseGuards(new PermissionGuard('device.force_unbind'))
    async forceUnbindDevice(
        @Param('id', ParseIntPipe) deviceId: number,
        @Body() dto: ForceUnbindDto
    ): Promise<void> {
        await this.adminService.forceUnbindDevice(deviceId, dto);
    }

    @Get(':id/logs')
    @UseGuards(new PermissionGuard('device.view_all'))
    async getDeviceLogs(
        @Param('id', ParseIntPipe) deviceId: number,
        @Query() query: LogsQueryDto
    ): Promise<DeviceLogDto[]> {
        return this.adminService.getDeviceLogs(deviceId, query);
    }

    @Post(':id/debug')
    @UseGuards(new PermissionGuard('device.debug'))
    async sendDebugCommand(
        @Param('id', ParseIntPipe) deviceId: number,
        @Body() dto: DebugCommandDto
    ): Promise<void> {
        await this.adminService.sendDebugCommand(deviceId, dto);
    }
}

export class AdminDeviceDto {
    id: number;
    uuid: string;
    name: string;
    mac: string;
    userId: number;
    username: string;
    isOnline: boolean;
    lastSeen: Date;
    bindStatus: string;
    firmwareVersion: string;
    ipAddress: string;
    wifiSsid: string;
    rssi: number;
    location: string;
}

export class AdminDeviceDetailDto extends AdminDeviceDto {
    config: any;
    sensorData: SensorReading[];
    recentCommands: CommandLog[];
    bindHistory: DeviceBinding[];
}

export class ForceUnbindDto {
    @IsString()
    reason: string;

    @IsOptional()
    @IsBoolean()
    notifyUser?: boolean;

    @IsOptional()
    @IsBoolean()
    backupConfig?: boolean;
}
```

### 5.4 系统统计API

```typescript
@Controller('admin/stats')
@UseGuards(JwtAuthGuard, AdminGuard)
export class AdminStatsController {
    constructor(private adminService: AdminService) {}

    @Get('dashboard')
    async getDashboardStats(): Promise<DashboardStatsDto> {
        return this.adminService.getDashboardStats();
    }

    @Get('users')
    async getUserStats(
        @Query('period') period: 'day' | 'week' | 'month' = 'day'
    ): Promise<UserStatsDto> {
        return this.adminService.getUserStats(period);
    }

    @Get('devices')
    async getDeviceStats(
        @Query('period') period: 'day' | 'week' | 'month' = 'day'
    ): Promise<DeviceStatsDto> {
        return this.adminService.getDeviceStats(period);
    }

    @Get('system')
    async getSystemStats(): Promise<SystemStatsDto> {
        return this.adminService.getSystemStats();
    }
}

export class DashboardStatsDto {
    totalUsers: number;
    activeUsersToday: number;
    totalDevices: number;
    onlineDevices: number;
    newUsersToday: number;
    newDevicesToday: number;
    apiCallsToday: number;
    alerts: AlertDto[];
}

export class UserStatsDto {
    total: number;
    active: number;
    inactive: number;
    byRole: Record<UserRole, number>;
    registrationTrend: { date: string; count: number }[];
}

export class DeviceStatsDto {
    total: number;
    online: number;
    offline: number;
    byBrand: Record<string, number>;
    byFirmware: Record<string, number>;
    connectionTrend: { date: string; online: number; offline: number }[];
}

export class SystemStatsDto {
    uptime: number;
    memory: {
        used: number;
        total: number;
    };
    database: {
        size: number;
        tables: Record<string, number>;
    };
    mqtt: {
        connectedClients: number;
        messagesPerMinute: number;
    };
}
```

### 5.5 管理服务实现

```typescript
@Injectable()
export class AdminService {
    constructor(
        @InjectRepository(User)
        private userRepository: Repository<User>,
        @InjectRepository(Device)
        private deviceRepository: Repository<Device>,
        private deviceBindingService: DeviceBindingService,
    ) {}

    async listUsers(query: ListUsersQueryDto): Promise<{
        users: AdminUserDto[];
        total: number;
        page: number;
        pageSize: number;
    }> {
        const qb = this.userRepository.createQueryBuilder('user')
            .leftJoinAndSelect('user.devices', 'device')
            .select([
                'user.id',
                'user.username',
                'user.email',
                'user.role',
                'user.isActive',
                'user.createdAt',
                'user.lastLoginAt',
            ])
            .addSelect('COUNT(device.id)', 'deviceCount')
            .groupBy('user.id');

        // 搜索
        if (query.search) {
            qb.andWhere(
                '(user.username LIKE :search OR user.email LIKE :search)',
                { search: `%${query.search}%` }
            );
        }

        // 筛选
        if (query.role) {
            qb.andWhere('user.role = :role', { role: query.role });
        }

        if (query.isActive !== undefined) {
            qb.andWhere('user.isActive = :isActive', { isActive: query.isActive });
        }

        // 排序
        qb.orderBy(`user.${query.sortBy}`, query.sortOrder);

        // 分页
        const [users, total] = await qb
            .skip((query.page - 1) * query.pageSize)
            .take(query.pageSize)
            .getRawMany();

        return {
            users: users.map(u => ({
                id: u.user_id,
                username: u.user_username,
                email: u.user_email,
                role: u.user_role,
                isActive: u.user_isActive,
                deviceCount: parseInt(u.deviceCount),
                createdAt: u.user_createdAt,
                lastLoginAt: u.user_lastLoginAt,
            })),
            total,
            page: query.page,
            pageSize: query.pageSize,
        };
    }

    async resetUserPassword(
        userId: number, 
        dto: ResetPasswordDto
    ): Promise<{ tempPassword: string }> {
        const tempPassword = dto.tempPassword || this.generateTempPassword();
        const hashedPassword = await bcrypt.hash(tempPassword, 10);

        await this.userRepository.update(userId, {
            password: hashedPassword,
            requirePasswordChange: dto.requireChange ?? true,
            passwordChangedAt: new Date(),
        });

        return { tempPassword };
    }

    private generateTempPassword(): string {
        const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
        let password = '';
        for (let i = 0; i < 12; i++) {
            password += chars.charAt(Math.floor(Math.random() * chars.length));
        }
        return password;
    }

    async getDashboardStats(): Promise<DashboardStatsDto> {
        const today = new Date();
        today.setHours(0, 0, 0, 0);

        const [
            totalUsers,
            activeUsersToday,
            totalDevices,
            onlineDevices,
            newUsersToday,
            newDevicesToday
        ] = await Promise.all([
            this.userRepository.count(),
            this.userRepository.count({ where: { lastLoginAt: MoreThanOrEqual(today) } }),
            this.deviceRepository.count(),
            this.deviceRepository.count({ where: { isOnline: true } }),
            this.userRepository.count({ where: { createdAt: MoreThanOrEqual(today) } }),
            this.deviceRepository.count({ where: { createdAt: MoreThanOrEqual(today) } }),
        ]);

        return {
            totalUsers,
            activeUsersToday,
            totalDevices,
            onlineDevices,
            newUsersToday,
            newDevicesToday,
            apiCallsToday: 0,  // 需要从日志统计
            alerts: [],  // 需要从告警系统获取
        };
    }
}
```

---

## 6. API接口规范

### 6.1 REST API汇总

```yaml
# ========== 认证与协议 ==========
POST   /auth/register                    # 注册（含协议）
POST   /auth/agreement/accept            # 接受协议
GET    /auth/agreement/current          # 获取当前协议

# ========== 用户管理 ==========
POST   /users/me/change-password         # 修改密码
GET    /users/me/password-status         # 密码状态

# ========== 管理员 - 用户管理 ==========
GET    /admin/users                     # 用户列表
GET    /admin/users/:id                 # 用户详情
PATCH  /admin/users/:id/status          # 更新用户状态
POST   /admin/users/:id/reset-password  # 重置密码
DELETE /admin/users/:id                 # 删除用户
GET    /admin/users/:id/devices         # 用户设备列表

# ========== 管理员 - 设备管理 ==========
GET    /admin/devices                   # 所有设备列表
GET    /admin/devices/:id               # 设备详情
POST   /admin/devices/:id/unbind        # 强制解绑
GET    /admin/devices/:id/logs          # 设备日志
POST   /admin/devices/:id/debug         # 发送调试命令

# ========== 管理员 - 系统统计 ==========
GET    /admin/stats/dashboard           # 仪表盘统计
GET    /admin/stats/users               # 用户统计
GET    /admin/stats/devices             # 设备统计
GET    /admin/stats/system              # 系统统计
```

### 6.2 错误码定义

```typescript
export const ERROR_CODES = {
    // 绑定相关
    DEVICE_ALREADY_BOUND: 'DEVICE_001',
    DEVICE_BOUND_TO_OTHER: 'DEVICE_002',
    DEVICE_NOT_REBINDABLE: 'DEVICE_003',
    BINDING_FAILED: 'DEVICE_004',

    // 协议相关
    AGREEMENT_REQUIRED: 'AGREEMENT_001',
    AGREEMENT_OUTDATED: 'AGREEMENT_002',

    // 密码相关
    PASSWORD_INCORRECT: 'PASSWORD_001',
    PASSWORD_POLICY_VIOLATION: 'PASSWORD_002',
    PASSWORD_REUSED: 'PASSWORD_003',
    PASSWORD_EXPIRED: 'PASSWORD_004',

    // 权限相关
    ADMIN_REQUIRED: 'ADMIN_001',
    PERMISSION_DENIED: 'ADMIN_002',
};
```

---

## 7. 实施顺序

### Week 7.1: 设备绑定改进
1. [ ] 创建DeviceBinding实体
2. [ ] 实现DeviceBindingService
3. [ ] 改进DevicesService绑定逻辑
4. [ ] 实现绑定历史记录
5. [ ] 添加防掉绑定机制

### Week 7.2: 用户协议系统
1. [ ] 创建UserAgreement实体
2. [ ] 实现AgreementService
3. [ ] 改进注册流程
4. [ ] 添加协议API
5. [ ] 创建默认协议内容

### Week 8.1: 密码系统
1. [ ] 创建PasswordHistory实体
2. [ ] 实现PasswordService
3. [ ] 实现密码策略验证
4. [ ] 添加密码API
5. [ ] 密码过期检查

### Week 8.2: 管理后台
1. [ ] 实现AdminGuard和PermissionGuard
2. [ ] 实现AdminUsersController
3. [ ] 实现AdminDevicesController
4. [ ] 实现AdminStatsController
5. [ ] 实现AdminService

### Week 9.1: 集成测试
1. [ ] 单元测试
2. [ ] 集成测试
3. [ ] 安全性测试
4. [ ] 性能测试

### Week 9.2: 文档与优化
1. [ ] API文档
2. [ ] 代码审查
3. [ ] 性能优化
4. [ ] 部署准备

---

## 8. 测试策略

### 8.1 单元测试

```typescript
// device-binding.service.spec.ts
describe('DeviceBindingService', () => {
    it('should prevent duplicate binding', async () => {
        // 测试重复绑定
    });

    it('should backup config on unbind', async () => {
        // 测试配置备份
    });

    it('should restore config on rebind', async () => {
        // 测试配置恢复
    });
});

// password.service.spec.ts
describe('PasswordService', () => {
    it('should enforce password policy', async () => {
        // 测试密码策略
    });

    it('should prevent password reuse', async () => {
        // 测试历史密码检查
    });
});
```

### 8.2 集成测试

```typescript
// admin.e2e-spec.ts
describe('Admin API', () => {
    it('should list users with pagination', async () => {
        // 测试用户列表
    });

    it('should force unbind device', async () => {
        // 测试强制解绑
    });
});
```

---

**下一步**: 创建Phase 5前端完善设计
