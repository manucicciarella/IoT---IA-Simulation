# -*- coding: utf-8 -*-
import json
import random

random.seed(42)

# Acciones:
# 0 = Normal
# 1 = Regar
# 2 = Fumigar
# 3 = Alerta crítica
# 4 = Inspeccionar

ACCIONES = {
    0: "Normal",
    1: "Regar",
    2: "Fumigar",
    3: "Alerta critica",
    4: "Inspeccionar"
}

# Rangos físicos reales de los sensores Wokwi:
#   temp      → DHT22, °C
#   hum_amb   → DHT22, %
#   hum_suelo → potenciómetro mapeado 0-100 %
#   radiacion → LDR normalizado en Node-RED a 0-100 %
#   viento    → potenciómetro mapeado 0-120 km/h
RANGOS = {
    "temp":      (0,   50),
    "hum_amb":   (0,  100),
    "hum_suelo": (0,  100),
    "radiacion": (0,  100),
    "viento":    (0,  120),
}


def normalizar(valor, minimo, maximo):
    return round((valor - minimo) / (maximo - minimo), 4)


def one_hot(clase):
    return {str(i): (1 if i == clase else 0) for i in range(5)}


def generar_dato_por_clase(clase):

    # 0 = Normal — condiciones estables, sin acción urgente
    if clase == 0:
        return {
            "temp":      random.uniform(18, 30),
            "hum_amb":   random.uniform(35, 65),
            "hum_suelo": random.uniform(45, 75),
            "radiacion": random.uniform(20, 65),
            "viento":    random.uniform(5,  35),   # calmo a brisa moderada
        }

    # 1 = Regar — suelo seco, sin otras condiciones de alarma
    elif clase == 1:
        return {
            "temp":      random.uniform(20, 38),
            "hum_amb":   random.uniform(20, 65),
            "hum_suelo": random.uniform(5,  30),
            "radiacion": random.uniform(35, 85),
            "viento":    random.uniform(0,  45),   # bajo/moderado, seguro para riego
        }

    # 2 = Fumigar — ambiente húmedo/cálido, viento bajo (requisito operativo)
    elif clase == 2:
        return {
            "temp":      random.uniform(22, 35),
            "hum_amb":   random.uniform(70, 100),
            "hum_suelo": random.uniform(40, 85),
            "radiacion": random.uniform(10, 65),
            "viento":    random.uniform(0,  25),   # < 25 km/h: seguro para fumigación
        }

    # 3 = Alerta crítica — dos sub-escenarios peligrosos
    elif clase == 3:
        caso = random.choice(["viento_extremo", "calor_sequia"])

        if caso == "viento_extremo":
            return {
                "temp":      random.uniform(10, 38),
                "hum_amb":   random.uniform(20, 85),
                "hum_suelo": random.uniform(20, 80),
                "radiacion": random.uniform(10, 90),
                "viento":    random.uniform(100, 120),  # tormenta severa (tope sensor 120)
            }
        else:  # calor_sequia
            return {
                "temp":      random.uniform(40, 50),
                "hum_amb":   random.uniform(0,  35),
                "hum_suelo": random.uniform(0,  18),
                "radiacion": random.uniform(75, 100),
                "viento":    random.uniform(0,  60),
            }

    # 4 = Inspeccionar — lecturas dudosas o anómalas, no necesariamente críticas
    elif clase == 4:
        caso = random.choice(["lectura_rara", "posible_plaga", "zona_dudosa"])

        if caso == "lectura_rara":
            return {
                "temp":      random.choice([
                                 random.uniform(0,  8),
                                 random.uniform(42, 50),
                             ]),
                "hum_amb":   random.uniform(20,  90),
                "hum_suelo": random.uniform(35,  90),
                "radiacion": random.uniform(0,  100),
                "viento":    random.uniform(20,  85),  # variable, moderado a fuerte
            }

        elif caso == "posible_plaga":
            return {
                "temp":      random.uniform(24, 36),
                "hum_amb":   random.uniform(65, 100),
                "hum_suelo": random.uniform(25,  50),
                "radiacion": random.uniform(50, 100),
                "viento":    random.uniform(25,  70),  # moderado
            }

        else:  # zona_dudosa
            return {
                "temp":      random.uniform(10, 40),
                "hum_amb":   random.uniform(0,  100),
                "hum_suelo": random.uniform(20,  90),
                "radiacion": random.uniform(0,  100),
                "viento":    random.uniform(60,  99),  # fuerte pero bajo el umbral de alerta
            }


def convertir_a_item(raw, clase):
    input_normalizado = {
        sensor: normalizar(valor, *RANGOS[sensor])
        for sensor, valor in raw.items()
    }
    return {"input": input_normalizado, "output": one_hot(clase)}


def generar_dataset(cantidad_por_clase=250):
    dataset = []
    for clase in range(5):
        for _ in range(cantidad_por_clase):
            raw = generar_dato_por_clase(clase)
            dataset.append(convertir_a_item(raw, clase))
    random.shuffle(dataset)
    return dataset


dataset = generar_dataset(cantidad_por_clase=250)

with open("nodered/dataset_agro_nodered_sin_bateria.json", "w") as f:
    json.dump(dataset, f, indent=2, ensure_ascii=False)

print("Dataset generado correctamente")
print("Cantidad total de ejemplos:", len(dataset))
print("Archivo creado: nodered/dataset_agro_nodered_sin_bateria.json")
print("\nEjemplo:")
print(json.dumps(dataset[0], indent=2, ensure_ascii=False))
