import { Entity, Column, PrimaryGeneratedColumn, OneToOne, JoinColumn } from 'typeorm';
import { User } from './user.entity';

@Entity()
export class UserSettings {
    @PrimaryGeneratedColumn()
    id: number;

    @Column({ default: 'C' })
    temperatureUnit: string; // 'C' | 'F'

    @Column({ default: 'dark' })
    theme: string; // 'dark' | 'light'

    @OneToOne(() => User, (user) => user.settings)
    @JoinColumn({ name: 'userId' })
    user: User;

    @Column({ unique: true })
    userId: number;
}
