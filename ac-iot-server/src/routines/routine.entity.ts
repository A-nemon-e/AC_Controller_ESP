import { Entity, Column, PrimaryGeneratedColumn, ManyToOne, JoinColumn, CreateDateColumn, UpdateDateColumn } from 'typeorm';
import { User } from '../users/user.entity';

@Entity()
export class Routine {
    @PrimaryGeneratedColumn()
    id: number;

    @Column()
    name: string;

    @Column({ default: true })
    enabled: boolean;

    // Optional Cron expression, e.g., "0 22 * * *"
    @Column({ nullable: true })
    cron: string;

    // JSON configuration for triggers
    // e.g. [{ type: 'temp', operator: '>', value: 28, deviceId: 1 }]
    @Column('simple-json', { nullable: true })
    triggers: any;

    // JSON configuration for actions
    // e.g. [{ deviceId: 1, command: { power: true } }]
    @Column('simple-json')
    actions: any;

    @Column({ nullable: true })
    lastRun: Date;

    @CreateDateColumn()
    createdAt: Date;

    @UpdateDateColumn()
    updatedAt: Date;

    @ManyToOne(() => User, (user) => user.id)
    @JoinColumn({ name: 'userId' })
    user: User;

    @Column()
    userId: number;
}
