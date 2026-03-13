import {
  Entity,
  Column,
  PrimaryGeneratedColumn,
  ManyToOne,
  JoinColumn,
  Index,
} from 'typeorm';
import { User } from '../users/user.entity';

@Entity()
export class Device {
  @PrimaryGeneratedColumn()
  id: number;

  @Column()
  name: string;

  @Column({ unique: true })
  uuid: string;

  @Column({ type: 'text', nullable: true })
  brandConfig: string;

  @Column('simple-json', { nullable: true })
  irConfig: any;

  @Column('simple-json', { nullable: true })
  lastState: any;

  @Column('simple-json', { nullable: true })
  micConfig: any;

  @Column({ default: 'uninitialized' })
  setupStatus: string;

  @Column('simple-json', { nullable: true })
  supportedBrands: string[];

  @Column({ default: false })
  enableCurrent: boolean;

  @ManyToOne(() => User, (user) => user.id)
  @JoinColumn({ name: 'userId' })
  user: User;

  @Column({ type: 'int', nullable: true })
  userId: number;

  @Column({ default: false })
  isOnline: boolean;

  @Column({ nullable: true })
  lastSeen: Date;

  @Column({ default: 'pending' })
  bindStatus: 'pending' | 'bound' | 'unbound' | 'error';

  @Column({ type: 'datetime', nullable: true })
  lastBindTime: Date;

  @Column({ type: 'datetime', nullable: true })
  lastUnbindTime: Date;

  @Column({ type: 'int', nullable: true })
  boundUserId: number;

  @Column({ default: false })
  isRebindable: boolean;

  @Column({ type: 'simple-json', nullable: true })
  backupConfig: any;
}
