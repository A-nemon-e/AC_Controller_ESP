import {
  Entity,
  Column,
  PrimaryGeneratedColumn,
  CreateDateColumn,
} from 'typeorm';

@Entity()
export class DeviceBinding {
  @PrimaryGeneratedColumn()
  id: number;

  @Column()
  deviceId: number;

  @Column()
  userId: number;

  @Column({ type: 'text' })
  action: 'bind' | 'unbind' | 'rebind';

  @Column({ type: 'text', nullable: true })
  reason: string;

  @CreateDateColumn()
  timestamp: Date;
}
