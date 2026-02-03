import { Injectable, OnModuleInit, Logger } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { EventEmitter2 } from '@nestjs/event-emitter';
import * as mqtt from 'mqtt';

@Injectable()
export class MqttService implements OnModuleInit {
    private client: mqtt.MqttClient;
    private readonly logger = new Logger(MqttService.name);

    constructor(
        private configService: ConfigService,
        private eventEmitter: EventEmitter2
    ) { }

    onModuleInit() {
        this.connect();
    }

    private connect() {
        const url = this.configService.get<string>('MQTT_URL');
        if (!url) {
            this.logger.error('MQTT_URL not defined in configuration');
            return;
        }

        this.logger.log(`Connecting to MQTT Broker using URL provided in config...`);
        // Connect to the broker
        this.client = mqtt.connect(url);

        this.client.on('connect', () => {
            this.logger.log('Successfully connected to MQTT Broker');
            this.subscribeToTopics();
        });

        this.client.on('error', (err) => {
            this.logger.error(`MQTT Connection Error: ${err.message}`);
        });

        this.client.on('message', (topic, payload) => {
            this.logger.log(`Received message on topic "${topic}": ${payload.toString()}`);
            this.eventEmitter.emit('mqtt.message', { topic, payload });
        });
    }

    private subscribeToTopics() {
        // Subscribe to all AC device topics for debugging
        // ac/user_{id}/dev_{id}/status
        const topicPattern = 'ac/+/+/status';
        this.client.subscribe(topicPattern, (err) => {
            if (err) {
                this.logger.error(`Failed to subscribe to ${topicPattern}`);
            } else {
                this.logger.log(`Subscribed to ${topicPattern}`);
            }
        });
    }

    publish(topic: string, message: string) {
        if (this.client && this.client.connected) {
            this.client.publish(topic, message);
        } else {
            this.logger.warn('MQTT Client not connected, cannot publish');
        }
    }
}
