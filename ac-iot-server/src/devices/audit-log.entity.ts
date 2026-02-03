import { Entity, Column, PrimaryGeneratedColumn, CreateDateColumn, ManyToOne, JoinColumn } from 'typeorm';
import { Device } from './device.entity';
import { User } from '../users/user.entity';

export enum AuditAction {
    CMD_API = 'CMD_API',       // App/API Control
    CMD_CRON = 'CMD_CRON',     // Scheduled Routine
    SYNC_IR = 'SYNC_IR',       // IR Remote Sync
    EVENT_GHOST = 'EVENT_GHOST', // Ghost Operation
    LEARN = 'LEARN',           // IR Learning
}

@Entity()
export class AuditLog {
    @PrimaryGeneratedColumn()
    id: number;

    @Column({
        type: 'simple-enum',
        enum: AuditAction,
    })
    action: AuditAction;

    @Column('simple-json', { nullable: true })
    details: any;

    @CreateDateColumn()
    timestamp: Date;

    @ManyToOne(() => Device, (device) => device.id, { onDelete: 'CASCADE' })
    @JoinColumn({ name: 'deviceId' })
    device: Device;

    @Column()
    deviceId: number;

    // Optional: User who triggered the action (nullable for system events)
    @ManyToOne(() => User, (user) => user.id, { nullable: true, onDelete: 'SET NULL' })
    @JoinColumn({ name: 'userId' })
    user: User;

    @Column({ nullable: true })
    userId: number;
}
