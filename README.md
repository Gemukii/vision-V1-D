# vision-V1-D

## Aperçu rapide
- Stack de supervision IoT légère pour la vision V1-D.
- Collecte via Telegraf, stockage InfluxDB, visualisation Grafana, orchestration Docker Compose.

## Schéma d'infra
```mermaid
flowchart LR
    subgraph TTN[The Things Network]
        MQTT[MQTT Broker<br/>eu1.cloud.thethings.network:1883]
    end

    subgraph Edge[Edge Device]
        DEVICE[Capteur LoRaWAN<br/>STM32 + LoRa]
    end

    subgraph Stack[Docker Compose Stack]
        TELEGRAF[Telegraf<br/>mqtt_consumer]
        INFLUX[(InfluxDB<br/>:8086)]
        GRAFANA[Grafana<br/>:3000]
        
        TELEGRAF -->|writes to bucket 'tasks'| INFLUX
        INFLUX -->|Flux queries| GRAFANA
    end

    DEVICE -.->|LoRaWAN uplink| MQTT
    MQTT -->|MQTT| TELEGRAF
    GRAFANA -->|HTTP :3000| USERS[Utilisateurs<br/>Dashboard Web]
```

## Pourquoi ces outils ?
- Docker Compose : déploiement reproductible multi-services, isolations simples, portabilité.
- Telegraf : agent léger, plugins variés (input vision/système), configuration unique en TOML.
- InfluxDB : base time-series performante, TTL et rétentions faciles, requêtes Flux/InfluxQL.
- Grafana : dashboards rapides, alerting intégré, provisioning code (datasources/dashboards).

## Démarrage succinct
1) Prérequis : Docker + Docker Compose installés.
2) Lancer : `docker-compose up -d` depuis la racine.
3) Grafana : http://localhost:3000 (admin/admin par défaut si non changé).
4) Telegraf : `docker exec -it v1d-telegraf telegraf --once` pour vérifier la collecte.
5) Données : InfluxDB accessible sur 8086 (voir creds dans docker-compose ou secrets locaux).


## Diagramme de séquence (flux nominal)
```mermaid
sequenceDiagram
    participant Device
    participant TTN as TTN MQTT (TLS 8883)
    participant Telegraf
    participant Influx as InfluxDB
    participant Grafana

    Device->>TTN: Uplink LoRaWAN
    TTN-->>Telegraf: MQTT message (JSON)
    Telegraf->>Influx: Write bucket "tasks" (token)
    Grafana-->>Influx: Flux query (token lecture)
    Grafana-->>User: Dashboard v1-d-vision
```
