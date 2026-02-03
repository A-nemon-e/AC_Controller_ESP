import { Entity, Column, PrimaryGeneratedColumn, CreateDateColumn, ManyToOne, JoinColumn } from 'typeorm';
import { Device } from './device.entity';

@Entity()
export class SensorReading {
    @PrimaryGeneratedColumn()
    id: number;

    @Column('float', { nullable: true })
    temperature: number;

    @Column('float', { nullable: true })
    humidity: number;

    @Column('float', { nullable: true })
    current: number; // RMS Current in Amps

    @CreateDateColumn()
    timestamp: Date;

    @ManyToOne(() => Device, (device) => device.id, { onDelete: 'CASCADE' })
    @JoinColumn({ name: 'deviceId' })
    device: Device;

    @Column()
    deviceId: number;
}
