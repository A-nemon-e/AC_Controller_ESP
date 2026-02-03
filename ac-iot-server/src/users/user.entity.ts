import { Entity, Column, PrimaryGeneratedColumn, OneToOne } from 'typeorm';
import { UserSettings } from './user-settings.entity';

@Entity()
export class User {
    @PrimaryGeneratedColumn()
    id: number;

    @Column({ unique: true })
    username: string;

    @Column()
    password: string;

    @Column({ default: false })
    isAdmin: boolean;

    @OneToOne(() => UserSettings, (settings) => settings.user, { cascade: true })
    settings: UserSettings;
}
