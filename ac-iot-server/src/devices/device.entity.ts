import { Entity, Column, PrimaryGeneratedColumn, ManyToOne, JoinColumn } from 'typeorm';
import { User } from '../users/user.entity';

@Entity()
export class Device {
    @PrimaryGeneratedColumn()
    id: number;

    @Column()
    name: string;

    @Column({ unique: true })
    uuid: string;

    // Stores brand ID, capabilities mask, etc.
    @Column({ type: 'text', nullable: true })
    brandConfig: string;

    @Column('simple-json', { nullable: true })
    irConfig: any; // Stores learned codes: { "custom_cool_26": "RAW..." }

    @Column('simple-json', { nullable: true })
    lastState: any; // Stores synced state: { power: true, mode: 'cool', ... }

    @Column('simple-json', { nullable: true })
    micConfig: any; // Stores mic sensitivity: { enabled: true, sensitivity: 80, beepDurationMs: 500, beepType: 'short' }

    // 设备初始化状态
    @Column({ default: 'uninitialized' })
    setupStatus: string; // 'uninitialized' | 'brand_selected' | 'learning' | 'ready'

    // 缓存固件支持的品牌列表 (From 'brands/list')
    @Column('simple-json', { nullable: true })
    supportedBrands: string[];

    // 电流互感器开关
    @Column({ default: false })
    enableCurrent: boolean;

    @ManyToOne(() => User, (user) => user.id)
    @JoinColumn({ name: 'userId' })
    user: User;

    @Column()
    userId: number;

    // ✅ 新增：在线状态 (LWT)
    @Column({ default: false })
    isOnline: boolean;

    @Column({ nullable: true })
    lastSeen: Date;
}
