import {
  Entity,
  Column,
  PrimaryGeneratedColumn,
  CreateDateColumn,
} from 'typeorm';

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
}
