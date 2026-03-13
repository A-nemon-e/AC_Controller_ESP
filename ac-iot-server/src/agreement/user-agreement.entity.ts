import {
  Entity,
  Column,
  PrimaryGeneratedColumn,
  Index,
  CreateDateColumn,
} from 'typeorm';

@Entity()
export class UserAgreement {
  @PrimaryGeneratedColumn()
  id: number;

  @Column()
  version: string;

  @Column({ type: 'text' })
  content: string;

  @Column({ default: false })
  isCurrent: boolean;

  @Column({ type: 'simple-json', nullable: true })
  changes: string[];

  @CreateDateColumn()
  createdAt: Date;

  @Column({ type: 'datetime', nullable: true })
  effectiveDate: Date;
}
