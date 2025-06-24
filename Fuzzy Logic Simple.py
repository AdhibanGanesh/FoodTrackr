import numpy as np
import skfuzzy as fuzz
from skfuzzy.control import Antecedent, Consequent, Rule, ControlSystem, ControlSystemSimulation

temperature = Antecedent(np.arange(0, 15, 1), 'temperature')
humidity = Antecedent(np.arange(0, 101, 1), 'humidity')
ph_level = Antecedent(np.arange(0, 14, 0.1), 'ph_level')
gas_levels = Antecedent(np.arange(0, 1001, 1), 'gas_levels')
spoilage_risk = Consequent(np.arange(0, 101, 1), 'spoilage_risk')

temperature['low'] = fuzz.trapmf(temperature.universe, [0, 0, 2, 5])
temperature['moderate'] = fuzz.trimf(temperature.universe, [3, 7, 10])
temperature['high'] = fuzz.trapmf(temperature.universe, [8, 12, 15, 15])

humidity['low'] = fuzz.trapmf(humidity.universe, [0, 0, 20, 40])
humidity['optimal'] = fuzz.trimf(humidity.universe, [30, 50, 70])
humidity['high'] = fuzz.trapmf(humidity.universe, [60, 80, 100, 100])

ph_level['fresh'] = fuzz.trapmf(ph_level.universe, [6.5, 7, 7, 7.5])
ph_level['slightly_acidic'] = fuzz.trimf(ph_level.universe, [5.5, 6.5, 7])
ph_level['spoiled'] = fuzz.trapmf(ph_level.universe, [0, 0, 5.5, 6.5])

gas_levels['low'] = fuzz.trapmf(gas_levels.universe, [0, 0, 50, 200])
gas_levels['moderate'] = fuzz.trimf(gas_levels.universe, [150, 400, 600])
gas_levels['high'] = fuzz.trapmf(gas_levels.universe, [500, 800, 1000, 1000])

spoilage_risk['low'] = fuzz.trapmf(spoilage_risk.universe, [0, 0, 20, 40])
spoilage_risk['medium'] = fuzz.trimf(spoilage_risk.universe, [30, 50, 70])
spoilage_risk['high'] = fuzz.trapmf(spoilage_risk.universe, [60, 80, 100, 100])

rules = [
    Rule(temperature['high'] & humidity['high'], spoilage_risk['high']),
    Rule(temperature['high'] & humidity['optimal'], spoilage_risk['medium']),
    Rule(temperature['moderate'] & humidity['high'], spoilage_risk['medium']),
    Rule(temperature['low'] & humidity['optimal'], spoilage_risk['low']),
    Rule(temperature['low'] & humidity['low'], spoilage_risk['low']),
    Rule(temperature['high'] & ph_level['slightly_acidic'], spoilage_risk['high']),
    Rule(temperature['moderate'] & ph_level['slightly_acidic'], spoilage_risk['medium']),
    Rule(temperature['low'] & ph_level['fresh'], spoilage_risk['low']),
    Rule(gas_levels['high'] & ph_level['slightly_acidic'], spoilage_risk['high']),
    Rule(gas_levels['moderate'] & ph_level['slightly_acidic'], spoilage_risk['medium']),
    Rule(humidity['high'] & gas_levels['high'], spoilage_risk['high']),
    Rule(humidity['optimal'] & gas_levels['moderate'], spoilage_risk['medium']),
    Rule(humidity['low'] & gas_levels['low'], spoilage_risk['low']),
]

def uv_sterilization(risk):
    threshold = 25
    if (risk>threshold):
        print("UV Sterilization activated...")
        print("Shelf Life Extended... Consume the edible item within 3 days!!")
    else:
        print("Sterilization process not required!!")
      
spoilage_control = ControlSystem(rules)
spoilage_simulation = ControlSystemSimulation(spoilage_control)

try:
    temp_input = float(input("Enter temperature (°C): "))
    hum_input = float(input("Enter humidity (%): "))
    ph_input = float(input("Enter pH level: "))
    gas_input = float(input("Enter gas levels (ppm): "))

    spoilage_simulation.input['temperature'] = temp_input
    spoilage_simulation.input['humidity'] = hum_input
    spoilage_simulation.input['ph_level'] = ph_input
    spoilage_simulation.input['gas_levels'] = gas_input

    spoilage_simulation.compute()
    risk = spoilage_simulation.output['spoilage_risk']
    print(f"Spoilage Risk: {risk:.2f}%")
    uv_sterilization(risk)

except ValueError as e:
    print("Invalid input. Please enter numeric values only.")
