import { Injectable, Logger, OnModuleInit } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import * as mqtt from 'mqtt';

@Injectable()
export class MqttService implements OnModuleInit {
  private readonly logger = new Logger(MqttService.name);
  private client: mqtt.MqttClient;

  constructor(private configService: ConfigService) {}

  onModuleInit() {
    this.connect();
  }

  private connect() {
    const host = this.configService.get<string>('MQTT_HOST', 'localhost');
    const port = this.configService.get<number>('MQTT_PORT', 1883);
    const username = this.configService.get<string>('MQTT_USERNAME');
    const password = this.configService.get<string>('MQTT_PASSWORD');

    const options: mqtt.IClientOptions = {
      host,
      port,
      protocol: 'mqtt',
      reconnectPeriod: 5000,
      connectTimeout: 30000,
    };

    if (username && password) {
      options.username = username;
      options.password = password;
    }

    this.client = mqtt.connect(options);

    this.client.on('connect', () => {
      this.logger.log('Connected to MQTT broker');
    });

    this.client.on('error', (error) => {
      this.logger.error('MQTT connection error:', error.message);
    });

    this.client.on('reconnect', () => {
      this.logger.log('Reconnecting to MQTT broker...');
    });
  }

  async publish(topic: string, message: any): Promise<void> {
    if (!this.client || !this.client.connected) {
      this.logger.warn('MQTT client not connected, message queued');
      return;
    }

    const payload = typeof message === 'string' ? message : JSON.stringify(message);
    
    return new Promise((resolve, reject) => {
      this.client.publish(topic, payload, { qos: 1 }, (error) => {
        if (error) {
          this.logger.error(`Failed to publish to ${topic}:`, error.message);
          reject(error);
        } else {
          this.logger.debug(`Published to ${topic}`);
          resolve();
        }
      });
    });
  }

  async subscribe(topic: string, callback: (message: any) => void): Promise<void> {
    if (!this.client || !this.client.connected) {
      throw new Error('MQTT client not connected');
    }

    return new Promise((resolve, reject) => {
      this.client.subscribe(topic, { qos: 1 }, (error) => {
        if (error) {
          reject(error);
        } else {
          this.client.on('message', (receivedTopic, message) => {
            if (receivedTopic === topic) {
              try {
                const data = JSON.parse(message.toString());
                callback(data);
              } catch {
                callback(message.toString());
              }
            }
          });
          resolve();
        }
      });
    });
  }
}