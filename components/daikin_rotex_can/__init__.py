import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, button, number, select, text_sensor, canbus, text
from esphome.const import *
from esphome.core import Lambda
from esphome.cpp_generator import MockObj
from esphome.cpp_types import std_ns
from esphome.components.canbus import CanbusComponent
from esphome import core
import subprocess
import logging
import os

translations = {
    "de": {  # German
        "heating": "Heizen",
        "cooling": "Kühlen",
        "standby": "Standby",
        "hot_water_production": "Warmwasserbereitung",
        "defrosting": "Abtauen",
        "optimized_defrosting": "Optimiertes Abtauen",
        "temperature_antifreeze": "T-Frostschutz",
        "temperature_antifreeze_off": "Aus",
        "off": "Aus",
        "on": "An",
        "night_only": "Nur bei Nacht",							  
        "monday": "Montag",
        "tuesday": "Dienstag",
        "wednesday": "Mittwoch",
        "thursday": "Donnerstag",
        "friday": "Freitag",
        "saturday": "Samstag",
        "sunday": "Sonntag",
        "mo_to_su": "Montag bis Sonntag",
        "lowering": "Absenken",
        "summer": "Sommer",
        "automatic_1": "Automatik 1",
        "automatic_2": "Automatik 2",
        "weather_dependent": "Witterungsgeführt",		
        "standby_mode": "Standby",
        "heating_mode": "Heizen",
        "cooling_mode": "Kühlen",
        "defrosting_mode": "Abtauen",
        "hot_water_production_mode": "Warmwasserbereitung",
        "sg_mode_1": "SG Modus 1",
        "sg_mode_2": "SG Modus 2",
        "fixed": "Fest",
        "no_additional_heat_generator": "Kein zusätzlicher Wärmeerzeuger",
        "optional_backup_heater": "Optionaler Backup-Heater",
        "wez_for_hot_water_and_heating": "WEZ für WW und HZ",
        "wez1_for_hot_water_wez2_for_heating": "WEZ1 für WW - WEZ2 für HZ",
        "err_0": "Kein Fehler",
        "err_E9001": "E9001 Rücklauffühler",
        "err_E9002": "E9002 Vorlauffühler",
        "err_E9003": "E9003 Frostschutzfunktion",
        "err_E9004": "E9004 Durchfluss",
        "err_E9005": "E9005 Vorlauftemperaturfühler",
        "err_E9006": "E9006 Vorlauftemperaturfühler",
        "err_E9007": "E9007 Platine IG defekt",
        "err_E9008": "E9008 Kältemitteltemperatur außerhalb des Bereiches",
        "err_E9009": "E9009 STB Fehler",
        "err_E9010": "E9010 STB Fehler",
        "err_E9011": "E9011 Fehler Flowsensor",
        "err_E9012": "E9012 Fehler Vorlauffühler",
        "err_E9013": "E9013 Platine AG defekt",
        "err_E9014": "E9014 P-Kältemittel hoch",
        "err_E9015": "E9015 P-Kältemittel niedrig",
        "err_E9016": "E9016 Lastschutz Verdichter",
        "err_E9017": "E9017 Ventilator blockiert",
        "err_E9018": "E9018 Expansionsventil",
        "err_E9019": "E9019 Warmwassertemperatur > 85°C",
        "err_E9020": "E9020 T-Verdampfer hoch",
        "err_E9021": "E9021 HPS-System",
        "err_E9022": "E9022 Fehler AT-Fühler",
        "err_E9023": "E9023 Fehler WW-Fühler",
        "err_E9024": "E9024 Drucksensor",
        "err_E9025": "E9025 Fehler Rücklauffühler",
        "err_E9026": "E9026 Drucksensor",
        "err_E9027": "E9027 Aircoil-Fühler Defrost",
        "err_E9028": "E9028 Aircoil-Fühler temp",
        "err_E9029": "E9029 Fehler Kältefühler AG",
        "err_E9030": "E9030 Defekt elektrisch",
        "err_E9031": "E9031 Defekt elektrisch",
        "err_E9032": "E9032 Defekt elektrisch",
        "err_E9033": "E9033 Defekt elektrisch",
        "err_E9034": "E9034 Defekt elektrisch",
        "err_E9035": "E9035 Platine AG defekt",
        "err_E9036": "E9036 Defekt elektrisch",
        "err_E9037": "E9037 Einstellung Leistung",
        "err_E9038": "E9038 Kältemittel Leck",
        "err_E9039": "E9039 Unter/Überspannung",
        "err_E9041": "E9041 Übertragungsfehler",
        "err_E9042": "E9042 Übertragungsfehler",
        "err_E9043": "E9043 Übertragungsfehler",
        "err_E9044": "E9044 Übertragungsfehler",
        "err_E75": "E75 Fehler Außentemperaturfühler",
        "err_E76": "E76 Fehler Speichertemperaturfühler",
        "err_E81": "E81 Kommunikationsfehler Rocon",
        "err_E88": "E88 Kommunikationsfehler Rocon Handbuch",
        "err_E91": "E91 Kommunikationsfehler Rocon Handbuch",
        "err_E128": "E128 Fehler Rücklauftemperaturfühler",
        "err_E129": "E129 Fehler Drucksensor",
        "err_E198": "E198 Durchflussmessung nicht plausibel",
        "err_E200": "E200 Kommunikationsfehler",
        "err_E8005": "E8005 Wasserdruck in Heizungsanlage zu gering",
        "err_E8100": "E8100 Kommunikation",
        "err_E9000": "E9000 Interne vorübergehende Meldung",
        "err_W8006": "W8006 Warnung Druckverlust",
        "err_W8007": "W8007 Wasserdruck in Anlage zu hoch",
        "defect": "Defekt",
        "sgn_normal_mode": "SGN - Normaler Modus",
        "sg1_hot_water_and_heating_off": "SG1 - WW & HZ ausgeschalten",
        "sg2_hot_water_and_heating_plus_5c": "SG2 - WW & HZ + 5°C",
        "sg3_hot_water_70c": "SG3 - WW 70°C",
        # Entities
        "ext": "Ext",
        "tv": "Heizkreis Vorlauf (TV)",
        "tvbh": "Vorlauftemperatur Heizung (TVBH)",
        "tr": "Ruecklauftemperatur Heizung",
        "temperature_outside": "Aussentemperatur",
        "tliq": "Tliq",
        "ta2": "TA2",
        "water_pressure": "Wasserdruck",
        "t_hs": "T-WE",
        "t_ext": "T-Aussen",
        "flow_rate": "Durchfluss",
        "status_kesselpumpe": "Status Kesselpumpe",
        "runtime_compressor": "Laufzeit Compressor",
        "runtime_pump": "Laufzeit Pump",
        "dhw_mixer_position": "DHW Mischer Position",
        "qboh": "EHS für DHW",
        "ehs_for_ch": "EHS fuer CH",
        "energy_cooling": "Energie Kühlung",
        "qch": "Energie Heizung",
        "total_energy_produced": "Erzeugte Energie Gesamt",
        "qdhw": "Energie für WW",
        "total_electrical_energy": "Elektrische Energie Gesamt",
        "energy_saving_mode": "ES mode",
        "operating_mode": "Betriebsmodus",
        "target_room1_temperature": "Raumsoll 1",
        "target_hot_water_temperature": "T-WW-Soll1",
        "1_dhw": "1 x Warmwasser",
        "hp_hyst_tdhw": "WPHyst TDHW",
        "delay_time_for_backup_heating": "Wartezeit BOH",
        "outdoor_unit": "Aussengerät",
        "indoor_unit": "Innengerät",
        "function_ehs": "Funktion EHS",
        "ch_support": "HZ Unterstützung",
        "smart_grid": "Smart Grid",
        "sg_mode": "SG Modus",
        "external_temp_sensor": "SKonfig T-Außen",
        "power_dhw": "Leistung WW",
        "power_ehs_1": "Leistung EHS Stufe 1",
        "power_ehs_2": "Leistung EHS Stufe 2",
        "power_biv": "Leistung BIV",
        "tdiff_dhw_ch": "TDiff-WW HZU",
        "max_heating_temperature": "Max Temp Heizung",
        "quiet": "Flüsterbetrieb",
        "t_dhw_1_min": "Schaltschwelle TDHW",
        "delta_temp_ch": "Spreizung HZ",
        "delta_temp_dhw": "Spreizung WW",
        "flow_rate_min": "Durchfluss Min",
        "flow_rate_setpoint": "Durchfluss Soll",
        "flow_rate_calc": "DurchflussBer",
        "flow_rate_hyst": "Durchfluss Hyst",
        "supply_temperature_adjustment_heating": "Anpassung T-VL Heizen",
        "supply_temperature_adjustment_cooling": "Anpassung T-VL Kühlen",
        "hk_function": "HK Funktion",
        "temperature_antifreeze": "T-Frostschutz",
        "heating_limit_day": "Heizgrenze Tag",
        "heating_limit_night": "Heizgrenze Nacht",
        "heating_curve": "Heizkurve",
        "flow_temperature_day": "T Vorlauf Tag",
        "flow_temperature_night": "T Vorlauf Nacht",
        "max_target_flow_temp": "Max T-Vorlauf",
        "min_target_flow_temp": "Min T-Vorlauf",
        "circulation_with_dhw_program": "Zirk mit WW-Prog",
        "circulation_interval_on": "ZirkInterval An",
        "circulation_interval_off": "ZirkInterval Aus",
        "antileg_day": "AntilegTag",
        "antileg_temp": "AntilegTemp",
        "max_dhw_loading": "Max WW Ladezeit",
        "dhw_off_time": "WW Sperrzeit",
        "electric_heater": "Heizstäbe - Für Pumpen nach Oktober 2018",
        "thermal_power": "Thermische Leistung",
        "optimized_defrosting": "Abtau-Optimierung",
        "bypass_valve": "BPV",
        "circulation_pump": "Umwaelzpumpe",
        "circulation_pump_min": "Umwälzpumpe Min",
        "circulation_pump_max": "Umwälzpumpe Max",
        "dhw_run": "Warmwasser bereiten",
        "error_code": "Fehlercode",
        "mode_of_operating": "Betriebsart",
        "status_kompressor": "Status Kompressor",
        "tdhw1": "Warmwassertemperatur",
        "target_supply_temperature": "Vorlauf Soll"            
    },
    "en": {  # English
        "heating": "Heating",
        "cooling": "Cooling",
        "standby": "Standby",
        "hot_water_production": "Hot Water Production",
        "defrosting": "Defrosting",
        "optimized_defrosting": "Optimized Defrosting",
        "temperature_antifreeze": "Antifreeze Temperature",
        "temperature_antifreeze_off": "Off",
        "off": "Off",
        "on": "On",
        "night_only": "Night Only",						   
        "monday": "Monday",
        "tuesday": "Tuesday",
        "wednesday": "Wednesday",
        "thursday": "Thursday",
        "friday": "Friday",
        "saturday": "Saturday",
        "sunday": "Sunday",
        "mo_to_su": "Monday to Sunday",
        "lowering": "Lowering",
        "summer": "Summer",
        "automatic_1": "Automatic 1",
        "automatic_2": "Automatic 2",
        "weather_dependent": "Weather Dependent",		
        "standby_mode": "Standby",
        "heating_mode": "Heating",
        "cooling_mode": "Cooling",
        "defrosting_mode": "Defrosting",
        "hot_water_production_mode": "Hot Water Production",
        "sg_mode_1": "SG Mode 1",
        "sg_mode_2": "SG Mode 2",
        "fixed": "Fixed",
        "no_additional_heat_generator": "No Additional Heat Generator",
        "optional_backup_heater": "Optional Backup Heater",
        "wez_for_hot_water_and_heating": "WEZ for Hot Water and Heating",
        "wez1_for_hot_water_wez2_for_heating": "WEZ1 for Hot Water - WEZ2 for Heating",
        "err_0": "No Error",
        "err_E9001": "E9001 Return Sensor",
        "err_E9002": "E9002 Supply Sensor",
        "err_E9003": "E9003 Frost Protection Function",
        "err_E9004": "E9004 Flow",
        "err_E9005": "E9005 Supply Temperature Sensor",
        "err_E9006": "E9006 Supply Temperature Sensor",
        "err_E9007": "E9007 IG Board Defective",
        "err_E9008": "E9008 Refrigerant Temperature Out of Range",
        "err_E9009": "E9009 STB Error",
        "err_E9010": "E9010 STB Error",
        "err_E9011": "E9011 Flow Sensor Error",
        "err_E9012": "E9012 Supply Sensor Error",
        "err_E9013": "E9013 AG Board Defective",
        "err_E9014": "E9014 High Refrigerant Pressure",
        "err_E9015": "E9015 Low Refrigerant Pressure",
        "err_E9016": "E9016 Compressor Overload Protection",
        "err_E9017": "E9017 Fan Blocked",
        "err_E9018": "E9018 Expansion Valve",
        "err_E9019": "E9019 Hot Water Temperature > 85°C",
        "err_E9020": "E9020 High Evaporator Temperature",
        "err_E9021": "E9021 HPS System",
        "err_E9022": "E9022 Ambient Temperature Sensor Error",
        "err_E9023": "E9023 Hot Water Sensor Error",
        "err_E9024": "E9024 Pressure Sensor",
        "err_E9025": "E9025 Return Sensor Error",
        "err_E9026": "E9026 Pressure Sensor",
        "err_E9027": "E9027 Air Coil Defrost Sensor",
        "err_E9028": "E9028 Air Coil Temperature Sensor",
        "err_E9029": "E9029 Cooling Sensor Error AG",
        "err_E9030": "E9030 Electrical Defect",
        "err_E9031": "E9031 Electrical Defect",
        "err_E9032": "E9032 Electrical Defect",
        "err_E9033": "E9033 Electrical Defect",
        "err_E9034": "E9034 Electrical Defect",
        "err_E9035": "E9035 AG Board Defective",
        "err_E9036": "E9036 Electrical Defect",
        "err_E9037": "E9037 Power Setting",
        "err_E9038": "E9038 Refrigerant Leak",
        "err_E9039": "E9039 Under/Over Voltage",
        "err_E9041": "E9041 Communication Error",
        "err_E9042": "E9042 Communication Error",
        "err_E9043": "E9043 Communication Error",
        "err_E9044": "E9044 Communication Error",
        "err_E75": "E75 Ambient Temperature Sensor Error",
        "err_E76": "E76 Storage Temperature Sensor Error",
        "err_E81": "E81 Communication Error Rocon",
        "err_E88": "E88 Communication Error Rocon Manual",
        "err_E91": "E91 Communication Error Rocon Manual",
        "err_E128": "E128 Return Temperature Sensor Error",
        "err_E129": "E129 Pressure Sensor Error",
        "err_E198": "E198 Flow Measurement Not Plausible",
        "err_E200": "E200 Communication Error",
        "err_E8005": "E8005 Water Pressure in Heating System Too Low",
        "err_E8100": "E8100 Communication",
        "err_E9000": "E9000 Temporary Internal Message",
        "err_W8006": "W8006 Pressure Loss Warning",
        "err_W8007": "W8007 Water Pressure in System Too High",
        "defect": "Defect",
        "sgn_normal_mode": "SGN - Normal Mode",
        "sg1_hot_water_and_heating_off": "SG1 - Hot Water and Heating Off",
        "sg2_hot_water_and_heating_plus_5c": "SG2 - Hot Water and Heating +5°C",
        "sg3_hot_water_70c": "SG3 - Hot Water 70°C",
        # Entities
        "ext": "Ext",
        "tv": "Heating Circuit Flow (TV)",
        "tvbh": "Flow Temperature Heating (TVBH)",
        "tr": "Return Temperature Heating",
        "temperature_outside": "Outside Temperature",
        "tliq": "Tliq",
        "ta2": "TA2",
        "water_pressure": "Water Pressure",
        "t_hs": "T-WE",
        "t_ext": "T-Outside",
        "flow_rate": "Flow Rate",
        "status_kesselpumpe": "Boiler Pump Status",
        "runtime_compressor": "Compressor Runtime",
        "runtime_pump": "Pump Runtime",
        "dhw_mixer_position": "DHW Mixer Position",
        "qboh": "EHS for DHW",
        "ehs_for_ch": "EHS for CH",
        "energy_cooling": "Cooling Energy",
        "qch": "Heating Energy",
        "total_energy_produced": "Total Energy Produced",
        "qdhw": "Energy for DHW",
        "total_electrical_energy": "Total Electrical Energy",
        "energy_saving_mode": "ES Mode",
        "operating_mode": "Operating Mode",
        "target_room1_temperature": "Target Room 1 Temperature",
        "target_hot_water_temperature": "Target Hot Water Temperature",
        "1_dhw": "1 x Domestic Hot Water",
        "hp_hyst_tdhw": "HP Hyst TDHW",
        "delay_time_for_backup_heating": "Waiting Time for Backup Heating",
        "outdoor_unit": "Outdoor Unit",
        "indoor_unit": "Indoor Unit",
        "function_ehs": "Function EHS",
        "ch_support": "CH Support",
        "smart_grid": "Smart Grid",
        "sg_mode": "SG Mode",
        "external_temp_sensor": "Config External Temperature",
        "power_dhw": "Power DHW",
        "power_ehs_1": "Power EHS Level 1",
        "power_ehs_2": "Power EHS Level 2",
        "power_biv": "Power BIV",
        "tdiff_dhw_ch": "TDiff DHW CH",
        "max_heating_temperature": "Max Heating Temperature",
        "quiet": "Silent Mode",
        "t_dhw_1_min": "Switch Threshold TDHW",
        "delta_temp_ch": "CH Spread",
        "delta_temp_dhw": "DHW Spread",
        "flow_rate_min": "Flow Rate Min",
        "flow_rate_setpoint": "Flow Rate Setpoint",
        "flow_rate_calc": "Flow Rate Calculated",
        "flow_rate_hyst": "Flow Rate Hysteresis",
        "supply_temperature_adjustment_heating": "Supply Temperature Adjustment Heating",
        "supply_temperature_adjustment_cooling": "Supply Temperature Adjustment Cooling",
        "hk_function": "HK Function",
        "temperature_antifreeze": "Anti-Freeze Temperature",
        "heating_limit_day": "Heating Limit Day",
        "heating_limit_night": "Heating Limit Night",
        "heating_curve": "Heating Curve",
        "flow_temperature_day": "Flow Temperature Day",
        "flow_temperature_night": "Flow Temperature Night",
        "max_target_flow_temp": "Max Target Flow Temperature",
        "min_target_flow_temp": "Min Target Flow Temperature",
        "circulation_with_dhw_program": "Circulation with DHW Program",
        "circulation_interval_on": "Circulation Interval On",
        "circulation_interval_off": "Circulation Interval Off",
        "antileg_day": "Anti-Legionella Day",
        "antileg_temp": "Anti-Legionella Temperature",
        "max_dhw_loading": "Max DHW Loading Time",
        "dhw_off_time": "DHW Off Time",
        "electric_heater": "Electric Heater - For Pumps After October 2018",
        "thermal_power": "Thermal Power",
        "optimized_defrosting": "Optimized Defrosting",
        "bypass_valve": "Bypass Valve",
        "circulation_pump": "Circulation Pump",
        "circulation_pump_min": "Circulation Pump Min",
        "circulation_pump_max": "Circulation Pump Max",
        "dhw_run": "DHW Run",
        "error_code": "Error Code",
        "mode_of_operating": "Operating Mode",
        "status_kompressor": "Compressor Status",
        "tdhw1": "Hot Water Temperature",
        "target_supply_temperature": "Target Supply Temperature"     
    },
    "it": {  # Italian
        "heating": "Riscaldamento",
        "cooling": "Raffreddamento",
        "standby": "Standby",
        "hot_water_production": "Produzione di Acqua Calda",
        "defrosting": "Sbrinamento",
        "optimized_defrosting": "Sbrinamento Ottimizzato",
        "temperature_antifreeze": "Temperatura Antigelo",
        "temperature_antifreeze_off": "Spento",
        "off": "Spento",
        "on": "Acceso",
        "night_only": "Solo di Notte",							  
        "monday": "Lunedì",
        "tuesday": "Martedì",
        "wednesday": "Mercoledì",
        "thursday": "Giovedì",
        "friday": "Venerdì",
        "saturday": "Sabato",
        "sunday": "Domenica",
        "mo_to_su": "Lunedì a Domenica",
        "lowering": "Abbassamento",
        "summer": "Estate",
        "automatic_1": "Automatico 1",
        "automatic_2": "Automatico 2",
        "weather_dependent": "Dipendente dal Tempo",		
        "standby_mode": "Pronto",
        "heating_mode": "Riscaldamento",
        "cooling_mode": "Raffreddamento",
        "defrosting_mode": "Sbrinamento",
        "hot_water_production_mode": "Produzione di Acqua Calda",
        "sg_mode_1": "Modalità SG 1",
        "sg_mode_2": "Modalità SG 2",
        "fixed": "Fisso",
        "no_additional_heat_generator": "Nessun Generatore di Calore Aggiuntivo",
        "optional_backup_heater": "Backup-Heater Opzionale",
        "wez_for_hot_water_and_heating": "WEZ per Acqua Calda e Riscaldamento",
        "wez1_for_hot_water_wez2_for_heating": "WEZ1 per Acqua Calda - WEZ2 per Riscaldamento",
        "err_0": "Nessun Errore",
        "err_E9001": "E9001 Sensore di Ritorno",
        "err_E9002": "E9002 Sensore di Mandata",
        "err_E9003": "E9003 Funzione Protezione Antigelo",
        "err_E9004": "E9004 Flusso",
        "err_E9005": "E9005 Sensore Temperatura Mandata",
        "err_E9006": "E9006 Sensore Temperatura Mandata",
        "err_E9007": "E9007 Scheda IG Difettosa",
        "err_E9008": "E9008 Temperatura Refrigerante Fuori Range",
        "err_E9009": "E9009 Errore STB",
        "err_E9010": "E9010 Errore STB",
        "err_E9011": "E9011 Errore Sensore di Flusso",
        "err_E9012": "E9012 Errore Sensore Mandata",
        "err_E9013": "E9013 Scheda AG Difettosa",
        "err_E9014": "E9014 Pressione Refrigerante Alta",
        "err_E9015": "E9015 Pressione Refrigerante Bassa",
        "err_E9016": "E9016 Protezione Sovraccarico Compressore",
        "err_E9017": "E9017 Ventilatore Bloccato",
        "err_E9018": "E9018 Valvola di Espansione",
        "err_E9019": "E9019 Temperatura Acqua Calda > 85°C",
        "err_E9020": "E9020 Temperatura Evaporatore Alta",
        "err_E9021": "E9021 Sistema HPS",
        "err_E9022": "E9022 Errore Sensore Temperatura Ambiente",
        "err_E9023": "E9023 Errore Sensore Acqua Calda",
        "err_E9024": "E9024 Sensore di Pressione",
        "err_E9025": "E9025 Errore Sensore di Ritorno",
        "err_E9026": "E9026 Sensore di Pressione",
        "err_E9027": "E9027 Sensore Sbrinamento Air Coil",
        "err_E9028": "E9028 Sensore Temperatura Air Coil",
        "err_E9029": "E9029 Errore Sensore Raffreddamento AG",
        "err_E9030": "E9030 Difetto Elettrico",
        "err_E9031": "E9031 Difetto Elettrico",
        "err_E9032": "E9032 Difetto Elettrico",
        "err_E9033": "E9033 Difetto Elettrico",
        "err_E9034": "E9034 Difetto Elettrico",
        "err_E9035": "E9035 Scheda AG Difettosa",
        "err_E9036": "E9036 Difetto Elettrico",
        "err_E9037": "E9037 Impostazione Potenza",
        "err_E9038": "E9038 Perdita di Refrigerante",
        "err_E9039": "E9039 Sotto/Sovratensione",
        "err_E9041": "E9041 Errore di Comunicazione",
        "err_E9042": "E9042 Errore di Comunicazione",
        "err_E9043": "E9043 Errore di Comunicazione",
        "err_E9044": "E9044 Errore di Comunicazione",
        "err_E75": "E75 Errore Sensore Temperatura Ambiente",
        "err_E76": "E76 Errore Sensore Temperatura Serbatoio",
        "err_E81": "E81 Errore di Comunicazione Rocon",
        "err_E88": "E88 Errore di Comunicazione Manuale Rocon",
        "err_E91": "E91 Errore di Comunicazione Manuale Rocon",
        "err_E128": "E128 Errore Sensore Temperatura di Ritorno",
        "err_E129": "E129 Errore Sensore di Pressione",
        "err_E198": "E198 Misurazione Flusso Non Plausibile",
        "err_E200": "E200 Errore di Comunicazione",
        "err_E8005": "E8005 Pressione dell'Acqua nel Sistema di Riscaldamento Troppo Bassa",
        "err_E8100": "E8100 Comunicazione",
        "err_E9000": "E9000 Messaggio Interno Temporaneo",
        "err_W8006": "W8006 Avviso di Perdita di Pressione",
        "err_W8007": "W8007 Pressione dell'Acqua nel Sistema Troppo Alta",
        "defect": "Malfunzionamento",
        "sgn_normal_mode": "SGN - Modalità Normale",
        "sg1_hot_water_and_heating_off": "SG1 - Acqua Calda e Riscaldamento Spenti",
        "sg2_hot_water_and_heating_plus_5c": "SG2 - Acqua Calda e Riscaldamento +5°C",
        "sg3_hot_water_70c": "SG3 - Acqua Calda 70°C",
        # Entities
        "ext": "Esterno",
        "tv": "Circuito Riscaldamento Mandata (TV)",
        "tvbh": "Temperatura Mandata Riscaldamento (TVBH)",
        "tr": "Temperatura Ritorno Riscaldamento",
        "temperature_outside": "Temperatura Esterna",
        "tliq": "Tliq",
        "ta2": "TA2",
        "water_pressure": "Pressione dell'Acqua",
        "t_hs": "T-WE",
        "t_ext": "T-Esterno",
        "flow_rate": "Portata",
        "status_kesselpumpe": "Stato Pompa Circolazione",
        "runtime_compressor": "Tempo di Funzionamento Compressore",
        "runtime_pump": "Tempo di Funzionamento Pompa",
        "dhw_mixer_position": "Posizione Miscelatore ACS",
        "qboh": "EHS per ACS",
        "ehs_for_ch": "EHS per CH",
        "energy_cooling": "Energia Raffreddamento",
        "qch": "Energia Riscaldamento",
        "total_energy_produced": "Energia Totale Prodotta",
        "qdhw": "Energia per ACS",
        "total_electrical_energy": "Energia Elettrica Totale",
        "energy_saving_mode": "Modalità Risparmio Energetico",
        "operating_mode": "Modalità di Funzionamento",
        "target_room1_temperature": "Temperatura Obiettivo Stanza 1",
        "target_hot_water_temperature": "Temperatura Obiettivo ACS",
        "1_dhw": "1 x Acqua Calda",
        "hp_hyst_tdhw": "HP Hyst TDHW",
        "delay_time_for_backup_heating": "Tempo di Attesa per Riscaldamento di Backup",
        "outdoor_unit": "Unità Esterna",
        "indoor_unit": "Unità Interna",
        "function_ehs": "Funzione EHS",
        "ch_support": "Supporto CH",
        "smart_grid": "Smart Grid",
        "sg_mode": "Modalità SG",
        "external_temp_sensor": "Sensore Temperatura Esterna",
        "power_dhw": "Potenza ACS",
        "power_ehs_1": "Potenza EHS Livello 1",
        "power_ehs_2": "Potenza EHS Livello 2",
        "power_biv": "Potenza BIV",
        "tdiff_dhw_ch": "TDiff ACS CH",
        "max_heating_temperature": "Temperatura Massima Riscaldamento",
        "quiet": "Modalità Silenziosa",
        "t_dhw_1_min": "Soglia di Commutazione TDHW",
        "delta_temp_ch": "Differenza Temperatura CH",
        "delta_temp_dhw": "Differenza Temperatura ACS",
        "flow_rate_min": "Portata Minima",
        "flow_rate_setpoint": "Setpoint Portata",
        "flow_rate_calc": "Portata Calcolata",
        "flow_rate_hyst": "Isteresi Portata",
        "supply_temperature_adjustment_heating": "Regolazione Temperatura Mandata Riscaldamento",
        "supply_temperature_adjustment_cooling": "Regolazione Temperatura Mandata Raffreddamento",
        "hk_function": "Funzione HK",
        "temperature_antifreeze": "Temperatura Antigelo",
        "heating_limit_day": "Limite Riscaldamento Giorno",
        "heating_limit_night": "Limite Riscaldamento Notte",
        "heating_curve": "Curva di Riscaldamento",
        "flow_temperature_day": "Temperatura Mandata Giorno",
        "flow_temperature_night": "Temperatura Mandata Notte",
        "max_target_flow_temp": "Temperatura Massima Mandata Obiettivo",
        "min_target_flow_temp": "Temperatura Minima Mandata Obiettivo",
        "circulation_with_dhw_program": "Circolazione con Programma ACS",
        "circulation_interval_on": "Intervallo Circolazione On",
        "circulation_interval_off": "Intervallo Circolazione Off",
        "antileg_day": "Giorno Anti-Legionella",
        "antileg_temp": "Temperatura Anti-Legionella",
        "max_dhw_loading": "Tempo Massimo di Caricamento ACS",
        "dhw_off_time": "Tempo di Spegnimento ACS",
        "electric_heater": "Resistenze Elettriche - Per Pompe dopo Ottobre 2018",
        "thermal_power": "Potenza Termica",
        "optimized_defrosting": "Sbrinamento Ottimizzato",
        "bypass_valve": "Valvola di Bypass",
        "circulation_pump": "Pompa di Circolazione",
        "circulation_pump_min": "Pompa di Circolazione Minima",
        "circulation_pump_max": "Pompa di Circolazione Massima",
        "dhw_run": "Esecuzione ACS",
        "error_code": "Codice di Errore",
        "mode_of_operating": "Stato operativo",
        "status_kompressor": "Stato Compressore",
        "tdhw1": "Temperatura Acqua Calda",
        "target_supply_temperature": "Temperatura Mandata Obiettivo"        
    },
}

_LOGGER = logging.getLogger(__name__)

CONF_LANGUAGE = 'language'
SUPPORTED_LANGUAGES = ['en', 'de', 'it']

# Current language
current_language = "de"
delayed_translate_tag = "DELAYED_TRANSLATE:"

def set_language(lang):
    global current_language
    if lang in translations:
        _LOGGER.info("[Translate] Setting language to '%s'", lang)
        current_language = lang
    else:
        _LOGGER.warning("[Translate] Language '%s' not found in dictionary. Falling back to English.", lang)
        current_language = "en"  # Fallback

def delayed_translate(key: str) -> str:
    return delayed_translate_tag + key

def translate(key: str) -> str:

    global current_language
    lang_translations = translations.get(current_language, translations.get("en", {}))

    if key in lang_translations:
        translated = lang_translations[key]
        _LOGGER.info("[Translate] Key '%s' found in language '%s' -> '%s'",key, current_language, translated)
        return translated

    if "en" in translations and key in translations["en"]:
        _LOGGER.warning(
            "[Translate] Key '%s' not found in language '%s'. Falling back to English.", 
            key, current_language
        )
        return translations["en"][key]
    _LOGGER.error(
        "[Translate] Key '%s' not found in language '%s' or in fallback language 'en'. Returning error message.", 
        key, current_language
    )
    return f"ERROR: Key '{key}' not found"

def apply_delayed_translate(key: str) -> str:
    if isinstance(key, str) and key.startswith(delayed_translate_tag):
        stripped_key = key[len(delayed_translate_tag):]
        return translate(stripped_key)
    return key

def apply_translation_to_mapping(mapping: dict) -> dict:
    return {key: apply_delayed_translate(value) for key, value in mapping.items()}

def apply_translation_to_entityname(yaml_sensor_conf, id):
    if "name" in yaml_sensor_conf and yaml_sensor_conf["name"].strip() == "auto":
        yaml_sensor_conf["name"] = translate(id)

# Generate translation.cpp, creating translation dictionary from python one
def generate_cpp_translations_for_language(translations, selected_language, keys_to_include=None):
    cpp_code = '#include "translations.h"\n'
    cpp_code += '#include <string>\n\n'
    cpp_code += '#include "esphome/core/log.h"\n\n'
    cpp_code += 'namespace esphome {\nnamespace daikin_rotex_can {\n\n'

    # check that selected language exists in dictionary
    if selected_language not in translations:
        raise ValueError(f"Selected language '{selected_language}' not found in translations dictionary.")

    _LOGGER.info(f"Building cpp translate dictionary for language: {selected_language}")

    # Reduces to only necessary records
    selected_translations = translations[selected_language]
    if keys_to_include:
        selected_translations = {key: value for key, value in selected_translations.items() if key in keys_to_include}

    # Cpp reduced dictionary
    cpp_code += f'static const Translation translations[] = {{\n'
    translation_entries = []
    for key, value in translations[selected_language].items():
        translation_entries.append(f'    {{"{key}", "{value}"}}')
    cpp_code += ',\n'.join(translation_entries)
    cpp_code += '\n};\n\n'

    # Cpp translation function
    cpp_code += (
        'std::string translate(const std::string &key) {\n'
        '    for (const auto &entry : translations) {\n'
        '        if (key == entry.key) {\n'
        '            ESP_LOGD("translate", "Key \'%s\' translated -> \'%s\'", key.c_str(), entry.value);\n'
        '            return entry.value;\n'
        '        }\n'
        '    }\n'
        '    ESP_LOGW("TRANSLATE", "Key \'%s\' not found", key.c_str());\n'
        '    return "ERROR: Key \'" + key + "\' not found";\n'
        '}\n\n'
    )

    cpp_code += '}  // namespace daikin_rotex_can\n'
    cpp_code += '}  // namespace esphome\n'
    return cpp_code

# Write translations.cpp file
def write_cpp_file(output_dir, selected_language):
    _LOGGER.info("Writing cpp translate file")
    cpp_code = generate_cpp_translations_for_language(translations, selected_language)
    output_path = os.path.join(output_dir, "translations.cpp")
    with open(output_path, "w") as f:
        f.write(cpp_code)
    _LOGGER.info(f"Generated {output_path}")    

daikin_rotex_can_ns = cg.esphome_ns.namespace('daikin_rotex_can')
DaikinRotexCanComponent = daikin_rotex_can_ns.class_('DaikinRotexCanComponent', cg.Component)

CanSelect = daikin_rotex_can_ns.class_("CanSelect", select.Select)
CanNumber = daikin_rotex_can_ns.class_("CanNumber", number.Number)
CanSensor = daikin_rotex_can_ns.class_("CanSensor", sensor.Sensor)
CanTextSensor = daikin_rotex_can_ns.class_("CanTextSensor", text_sensor.TextSensor)
CanBinarySensor = daikin_rotex_can_ns.class_("CanBinarySensor", binary_sensor.BinarySensor)

LogFilterText = daikin_rotex_can_ns.class_("LogFilterText", text.Text)
CustomRequestText = daikin_rotex_can_ns.class_("CustomRequestText", text.Text)

DHWRunButton = daikin_rotex_can_ns.class_("DHWRunButton", button.Button)
DumpButton = daikin_rotex_can_ns.class_("DumpButton", button.Button)

UNIT_BAR = "bar"
UNIT_LITER_PER_HOUR = "L/h"
UNIT_LITER_PER_MIN = "L/min"

########## Icons ##########
ICON_SUN_SNOWFLAKE_VARIANT = "mdi:sun-snowflake-variant"

result = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], stdout=subprocess.PIPE, text=True, cwd=os.path.dirname(os.path.realpath(__file__)))
git_hash = result.stdout.strip()
_LOGGER.info("Project Git Hash %s", git_hash)

########## Configuration of Sensors, TextSensors, BinarySensors, Selects and Numbers ##########

sensor_configuration = [
   {
        "type": "select",
        "name": "1_dhw" ,
        "icon": "mdi:hand-water",
        "command": "31 00 FA 01 44 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("off"),
            0x01: delayed_translate("on")
        }
    },
    {
        "type": "number",
        "name": "hp_hyst_tdhw",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_KELVIN,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:arrow-left-right",
        "min_value": 2,
        "max_value": 20,
        "step": 0.1,
        "command": "31 00 FA 06 91 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "number",
        "name": "delay_time_for_backup_heating",
        "unit_of_measurement": UNIT_MINUTE,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:clock-time-two-outline",
        "min_value": 20,
        "max_value": 95,
        "step": 1,
        "command": "31 00 FA 06 92 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "select",
        "name": "outdoor_unit" ,
        "icon": ICON_SUN_SNOWFLAKE_VARIANT,
        "command": "31 00 FA 06 9A 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: "--",
            0x01: "4",
            0x02: "6",
            0x03: "8",
            0x04: "11",
            0x05: "14",
            0x06: "16"
        }
    },
    {
        "type": "select",
        "name": "indoor_unit" ,
        "icon": ICON_SUN_SNOWFLAKE_VARIANT,
        "command": "31 00 FA 06 99 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: "--",
            0x01: "304",
            0x02: "308",
            0x03: "508",
            0x04: "516"
        }
    },
    {
        "type": "number",
        "name": "antileg_temp",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 60,
        "max_value": 75,
        "step": 1,
        "command": "31 00 FA 05 87 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "select",
        "name": "antileg_day" ,
        "icon": ICON_SUN_SNOWFLAKE_VARIANT,
        "command": "31 00 FA 01 01 00 00",
        "data_offset": 5,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("off"),
            0x01: delayed_translate("monday"),
            0x02: delayed_translate("tuesday"),
            0x03: delayed_translate("wednesday"),
            0x04: delayed_translate("thursday"),
            0x05: delayed_translate("friday"),
            0x06: delayed_translate("saturday"),
            0x07: delayed_translate("sunday"),
            0x08: delayed_translate("mo_to_su")
        }
    },
    {
        "type": "number",
        "name": "circulation_interval_on",
        "unit_of_measurement": UNIT_MINUTE,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 0,
        "max_value": 15,
        "step": 1,
        "command": "31 00 FA 06 5E 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "number",
        "name": "circulation_interval_off",
        "unit_of_measurement": UNIT_MINUTE,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 0,
        "max_value": 15,
        "step": 1,
        "command": "31 00 FA 06 5F 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "select",
        "name": "circulation_with_dhw_program" ,
        "icon": ICON_SUN_SNOWFLAKE_VARIANT,
        "command": "31 00 FA 01 82 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("off"),
            0x01: delayed_translate("on")
        }
    },
    {
        "type": "number",
        "name": "t_dhw_1_min",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 20,
        "max_value": 85,
        "step": 1,
        "command": "31 00 FA 06 73 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "number",
        "name": "max_dhw_loading",
        "unit_of_measurement": UNIT_MINUTE,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 10,
        "max_value": 240,
        "step": 1,
        "command": "31 00 FA 01 80 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "number",
        "name": "dhw_off_time",
        "unit_of_measurement": UNIT_MINUTE,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 0,
        "max_value": 180,
        "step": 1,
        "command": "31 00 FA 4E 3F 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "number",
        "name": "tdiff_dhw_ch",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_KELVIN,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 2,
        "max_value": 15,
        "step": 1,
        "command": "31 00 FA 06 6D 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "sensor",
        "name": "t_hs",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "command": "31 00 FA 01 D6 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "range": [1, 90]
    },
    {
        "type": "sensor",
        "name": "temperature_outside",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "command": "31 00 FA C0 FF 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "signed": True,
        "range": [-30, 90]
    },
    {
        "type": "sensor",
        "name": "ta2",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "command": "31 00 FA C1 05",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "signed": True,
        "range": [-30, 90]
    },
    {
        "type": "sensor",
        "name": "tliq",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "command": "31 00 FA C1 03 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "signed": True,
        "range": [-30, 90]
    },
    {
        "type": "sensor",
        "name": "t_ext",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "can_id": 0x300,
        "command": "61 00 FA 0A 0C 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "signed": True,
        "range": [-30, 90]
    },
    {
        "type": "sensor",
        "name": "tdhw1",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "command": "31 00 FA 00 0E 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "range": [1, 90]
    },
    {
        "type": "sensor",
        "name": "water_pressure",
        "device_class": DEVICE_CLASS_PRESSURE,
        "unit_of_measurement": UNIT_BAR,
        "accuracy_decimals": 2,
        "state_class": STATE_CLASS_MEASUREMENT,
        "command": "31 00 1C 00 00 00 00",
        "data_offset": 3,
        "data_size": 2,
        "divider": 1000.0
    },
    {
        "type": "sensor",
        "name": "circulation_pump",
        "unit_of_measurement": UNIT_PERCENT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:pump",
        "command": "31 00 FA C0 F7 00 00",
        "data_offset": 6,
        "data_size": 1,
        "divider": 1,
        "range": [0, 100]
    },
    {
        "type": "number",
        "name": "circulation_pump_min",
        "unit_of_measurement": UNIT_PERCENT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-left",
        "min_value": 40,
        "max_value": 100,
        "step": 1,
        "command": "31 00 FA 06 7F 00 00",
        "data_offset": 6,
        "data_size": 1,
        "divider": 1
    },
    {
        "type": "number",
        "name": "circulation_pump_max",
        "unit_of_measurement": UNIT_PERCENT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-right",
        "min_value": 60,
        "max_value": 100,
        "step": 1,
        "command": "31 00 FA 06 7E 00 00",
        "data_offset": 6,
        "data_size": 1,
        "divider": 1
    },
    {
        "type": "sensor",
        "name": "bypass_valve",
        "unit_of_measurement": UNIT_PERCENT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:pipe-valve",
        "command": "31 00 FA C0 FB 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1,
        "range": [0, 100]
    },
    {
        "type": "sensor",
        "name": "dhw_mixer_position",
        "unit_of_measurement": UNIT_PERCENT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-left",
        "command": "31 00 FA 06 9B 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1,
        "range": [0, 100]
    },
    {
        "type": "sensor",
        "name": "target_supply_temperature",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "command": "31 00 02 00 00 00 00",
        "data_offset": 3,
        "data_size": 2,
        "divider": 10.0,
        "range": [0, 90]
    },
    {
        "type": "sensor",
        "name": "ehs_for_ch",
        "device_class": DEVICE_CLASS_ENERGY_STORAGE,
        "unit_of_measurement": UNIT_KILOWATT_HOURS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:transmission-tower",
        "command": "31 00 FA 09 20 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "sensor",
        "name": "qch",
        "device_class": DEVICE_CLASS_ENERGY_STORAGE,
        "unit_of_measurement": UNIT_KILOWATT_HOURS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:transmission-tower",
        "command": "31 00 FA 06 A7 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "sensor",
        "name": "qboh",
        "device_class": DEVICE_CLASS_ENERGY_STORAGE,
        "unit_of_measurement": UNIT_KILOWATT_HOURS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:transmission-tower",
        "command": "31 00 FA 09 1C 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "sensor",
        "name": "qdhw",
        "device_class": DEVICE_CLASS_ENERGY_STORAGE,
        "unit_of_measurement": UNIT_KILOWATT_HOURS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:transmission-tower",
        "command": "31 00 FA 09 2C 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "sensor",
        "name": "total_energy_produced",
        "device_class": DEVICE_CLASS_ENERGY_STORAGE,
        "unit_of_measurement": UNIT_KILOWATT_HOURS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:transmission-tower",
        "command": "31 00 FA 09 30 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "sensor",
        "name": "energy_cooling",
        "device_class": DEVICE_CLASS_ENERGY_STORAGE,
        "unit_of_measurement": UNIT_KILOWATT_HOURS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:transmission-tower",
        "command": "31 00 FA 06 A6 00 00",
        "data_offset": 5,
        "data_size": 2
    },
    {
        "type": "sensor",
        "name": "total_electrical_energy",
        "device_class": DEVICE_CLASS_ENERGY_STORAGE,
        "unit_of_measurement": UNIT_KILOWATT_HOURS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:transmission-tower",
        "command": "31 00 FA C2 FA 00 00",
        "data_offset": 5,
        "data_size": 2
    },
    {
        "type": "sensor",
        "name": "runtime_compressor",
        "unit_of_measurement": UNIT_HOUR,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:clock-time-two-outline",
        "command": "31 00 FA 06 A5 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "sensor",
        "name": "runtime_pump",
        "unit_of_measurement": UNIT_HOUR,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:clock-time-two-outline",
        "command": "31 00 FA 06 A4 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1
    },
    {
        "type": "number",
        "name": "delta_temp_ch",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_KELVIN,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:thermometer-lines",
        "min_value": 2,
        "max_value": 20,
        "step": 1,
        "command": "31 00 FA 06 83 00 00",
        "data_offset": 6,
        "data_size": 1,
        "divider": 10.0
    },
    {
        "type": "number",
        "name": "delta_temp_dhw",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_KELVIN,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:thermometer-lines",
        "min_value": 2,
        "max_value": 20,
        "step": 1,
        "command": "31 00 FA 06 84 00 00",
        "data_offset": 6,
        "data_size": 1,
        "divider": 10.0
    },
    {
        "type": "select",
        "name": "temperature_antifreeze",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "command": "31 00 FA 0A 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "map": {0xFF60 / 10.0: delayed_translate("off"), **{i: f"{i} °C" for i in range(-15, 6)}}
    },
    {
        "type": "sensor",
        "name": "tv",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:thermometer-lines",
        "command": "31 00 FA C0 FC 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "update_entity": "thermal_power",
        "range": [1, 90]
    },
    {
        "type": "sensor",
        "name": "tvbh",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:thermometer-lines",
        "command": "31 00 FA C0 FE 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "update_entity": "thermal_power",
        "range": [1, 90]
    },
    {
        "type": "sensor",
        "name": "tr",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:thermometer-lines",
        "command": "31 00 FA C1 00 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "update_entity": "thermal_power",
        "range": [1, 90]
    },
    {
        "type": "sensor",
        "name": "flow_rate",
        "unit_of_measurement": UNIT_LITER_PER_HOUR,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-right",
        "command": "31 00 FA 01 DA 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 1,
        "update_entity": "thermal_power",
        "range": [0, 3000]
    },
    {
        "type": "sensor",
        "name": "flow_rate_calc",
        "unit_of_measurement": UNIT_LITER_PER_MIN,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-right",
        "command": "31 00 FA 06 9C 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "number",
        "name": "flow_rate_setpoint",
        "unit_of_measurement": UNIT_LITER_PER_MIN,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-right",
        "min_value": 8,
        "max_value": 25,
        "step": 1,
        "command": "31 00 FA 06 89",
        "data_offset": 6,
        "data_size": 1,
        "divider": 10,
        "map": {
            "8": 8,
            "9": 9,
            "10": 10,
            "11": 11,
            "12": 12,
            "13": 13,
            "14": 14,
            "15": 15,
            "16": 16,
            "17": 17,
            "18": 18,
            "19": 19,
            "20": 20,
            "21": 21,
            "22": 22,
            "23": 23,
            "24": 24,
            "25": 25
        }
    },
    {
        "type": "number",
        "name": "flow_rate_min",
        "unit_of_measurement": UNIT_LITER_PER_MIN,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-right",
        "min_value": 12,
        "max_value": 25,
        "step": 1,
        "command": "31 00 FA 06 88",
        "data_offset": 6,
        "data_size": 1,
        "divider": 10,
        "map": {
            "12": 12,
            "13": 13,
            "14": 14,
            "15": 15,
            "16": 16,
            "17": 17,
            "18": 18,
            "19": 19,
            "20": 20,
            "21": 21,
            "22": 22,
            "23": 23,
            "24": 24,
            "25": 25
        }
    },
    {
        "type": "number",
        "name": "flow_rate_hyst",
        "unit_of_measurement": UNIT_LITER_PER_MIN,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-right",
        "min_value": 0,
        "max_value": 5,
        "step": 0.1,
        "command": "31 00 FA 06 8A",
        "data_offset": 6,
        "data_size": 1,
        "divider": 10
    },
    {
        "type": "number",
        "name": "target_room1_temperature",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 15,
        "max_value": 25,
        "step": 0.1,
        "command": "31 00 05 00 00 00 00",
        "data_offset": 3,
        "data_size": 2,
        "divider": 10.0,
        "map": {
            15: "15 °C",
            16: "16 °C",
            17: "17 °C",
            18: "18 °C",
            19: "19 °C",
            20: "20 °C",
            21: "21 °C",
            22: "22 °C",
            23: "23 °C",
            24: "24 °C",
            25: "25 °C"
        }
    },
    {
        "type": "number",
        "name": "flow_temperature_day",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 20,
        "max_value": 90,
        "step": 0.1,
        "command": "31 00 FA 01 29 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "number",
        "name": "flow_temperature_night",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 10,
        "max_value": 90,
        "step": 0.1,
        "command": "31 00 FA 01 2A 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "select",
        "name": "heating_limit_day",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 0,
        "max_value": 40,
        "step": 1,
        "command": "31 00 FA 01 16",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "map": {0xFE70 / 10.0: delayed_translate("off"), **{i: f"{i} °C" for i in range(10, 41)}}
    },
    {
        "type": "select",
        "name": "heating_limit_night",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 0,
        "max_value": 40,
        "step": 1,
        "command": "31 00 FA 01 17",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0,
        "map": {0x5A / 10.0: delayed_translate("off"), **{i: f"{i} °C" for i in range(10, 41)}}
    },
    {
        "type": "number",
        "name": "heating_curve",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 2,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 0,
        "max_value": 2.55,
        "step": 0.01,
        "command": "31 00 FA 01 0E 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 100.0
    },
    {
        "type": "number",
        "name": "min_target_flow_temp",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-left",
        "min_value": 10,
        "max_value": 90,
        "step": 1,
        "command": "31 00 FA 01 2B 00 00",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "number",
        "name": "max_target_flow_temp",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-right",
        "min_value": 20,
        "max_value": 90,
        "step": 1,
        "command": "31 00 28 00 00 00 00",
        "data_offset": 3,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "number",
        "name": "target_hot_water_temperature",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-right",
        "min_value": 35,
        "max_value": 70,
        "step": 1,
        "command": "31 00 13 00 00 00 00",
        "data_offset": 3,
        "data_size": 2,
        "divider": 10.0,
        "map": {
            35: "35 °C",
            40: "40 °C",
            45: "45 °C",
            48: "48 °C",
            49: "49 °C",
            50: "50 °C",
            51: "51 °C",
            52: "52 °C",
            60: "60 °C",
            70: "70 °C",
        }
    },
    {
        "type": "text_sensor",
        "name": "mode_of_operating" ,
        "icon": ICON_SUN_SNOWFLAKE_VARIANT,
        "command": "31 00 FA C0 F6 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("standby"),
            0x01: delayed_translate("heating"),
            0x02: delayed_translate("cooling"),
            0x03: delayed_translate("defrosting"),
            0x04: delayed_translate("hot_water_production")
        },
        "update_entity": "thermal_power"
    },
    {
        "type": "select",
        "name": "operating_mode" ,
        "icon": ICON_SUN_SNOWFLAKE_VARIANT,
        "command": "31 00 FA 01 12 00 00",
        "data_offset": 5,
        "data_size": 1,
        "map": {
            0x01: delayed_translate("standby"),
            0x03: delayed_translate("heating"),
            0x04: delayed_translate("lowering"),
            0x05: delayed_translate("summer"),
            0x11: delayed_translate("cooling"),
            0x0B: delayed_translate("automatic_1"),
            0x0C: delayed_translate("automatic_2"),
        }
    },
    {
        "type": "select",
        "name": "quiet" ,
        "icon": "mdi:weather-partly-cloudy",
        "command": "31 00 FA 06 96",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("off"),
            0x01: delayed_translate("on"),
            0x02: delayed_translate("night_only")
        }
    },
    {
        "type": "text_sensor",
        "name": "error_code" ,
        "icon": "mdi:alert",
        "command": "31 00 FA 13 88 00 00",
        "data_offset": 5,
        "data_size": 2,
        "map": {
            0: delayed_translate("err_0"),
            9001: delayed_translate("err_E9001"),
            9002: delayed_translate("err_E9002"),
            9003: delayed_translate("err_E9003"),
            9004: delayed_translate("err_E9004"),
            9005: delayed_translate("err_E9005"),
            9006: delayed_translate("err_E9006"),
            9007: delayed_translate("err_E9007"),
            9008: delayed_translate("err_E9008"),
            9009: delayed_translate("err_E9009"),
            9010: delayed_translate("err_E9010"),
            9011: delayed_translate("err_E9011"),
            9012: delayed_translate("err_E9012"),
            9013: delayed_translate("err_E9013"),
            9014: delayed_translate("err_E9014"),
            9015: delayed_translate("err_E9015"),
            9016: delayed_translate("err_E9016"),
            9017: delayed_translate("err_E9017"),
            9018: delayed_translate("err_E9018"),
            9019: delayed_translate("err_E9019"),
            9020: delayed_translate("err_E9020"),
            9021: delayed_translate("err_E9021"),
            9022: delayed_translate("err_E9022"),
            9023: delayed_translate("err_E9023"),
            9024: delayed_translate("err_E9024"),
            9025: delayed_translate("err_E9025"),
            9026: delayed_translate("err_E9026"),
            9027: delayed_translate("err_E9027"),
            9028: delayed_translate("err_E9028"),
            9029: delayed_translate("err_E9029"),
            9030: delayed_translate("err_E9030"),
            9031: delayed_translate("err_E9031"),
            9032: delayed_translate("err_E9032"),
            9033: delayed_translate("err_E9033"),
            9034: delayed_translate("err_E9034"),
            9035: delayed_translate("err_E9035"),
            9036: delayed_translate("err_E9036"),
            9037: delayed_translate("err_E9037"),
            9038: delayed_translate("err_E9038"),
            9039: delayed_translate("err_E9039"),
            9041: delayed_translate("err_E9041"),
            9042: delayed_translate("err_E9042"),
            9043: delayed_translate("err_E9043"),
            9044: delayed_translate("err_E9044"),
            75: delayed_translate("err_E75"),
            76: delayed_translate("err_E76"),
            81: delayed_translate("err_E81"),
            88: delayed_translate("err_E88"),
            91: delayed_translate("err_E91"),
            128: delayed_translate("err_E128"),
            129: delayed_translate("err_E129"),
            198: delayed_translate("err_E198"),
            200: delayed_translate("err_E200"),
            8005: delayed_translate("err_E8005"),
            8100: delayed_translate("err_E8100"),
            9000: delayed_translate("err_E9000"),
            8006: delayed_translate("err_W8006"),
            8007: delayed_translate("err_W8007")
        }
    },
    {
        "type": "binary_sensor",
        "name": "status_kompressor" ,
        "icon": "mdi:pump",
        "can_id": 0x500,
        "command": "A1 00 61 00 00 00 00",
        "data_offset": 3,
        "data_size": 1
    },
    {
        "type": "binary_sensor",
        "name": "status_kesselpumpe" ,
        "icon": "mdi:pump",
        "command": "31 00 FA 0A 8C 00 00",
        "data_offset": 6,
        "data_size": 1
    },
    {
        "type": "binary_sensor",
        "name": "external_temp_sensor" ,
        "icon": "mdi:pump",
        "command": "31 00 FA 09 61 00 00",
        "data_offset": 6,
        "data_size": 1,
        "handle_lambda": """
            return data[6] == 0x05;
        """
    },
    {
        "type": "binary_sensor",
        "name": "energy_saving_mode" ,
        "icon": "mdi:pump",
        "command": "31 00 FA 01 76 00 00",
        "data_offset": 6,
        "data_size": 1
    },
    {
        "type": "select",
        "name": "hk_function" ,
        "icon": "mdi:weather-partly-cloudy",
        "command": "31 00 FA 01 41 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("weather_dependent"),
            0x01: delayed_translate("fixed")
        }
    },
    {
        "type": "select",
        "name": "sg_mode" ,
        "icon": "mdi:weather-partly-cloudy",
        "command": "31 00 FA 06 94 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("off"),
            0x01: delayed_translate("sg_mode_1"),
            0x02: delayed_translate("sg_mode_2")
        }
    },
    {
        "type": "select",
        "name": "smart_grid" ,
        "icon": "mdi:weather-partly-cloudy",
        "command": "31 00 FA 06 93 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("off"),
            0x01: delayed_translate("on")
        }
    },
    {
        "type": "select",
        "name": "function_ehs" ,
        "icon": ICON_SUN_SNOWFLAKE_VARIANT,
        "command": "31 00 FA 06 D2 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("no_additional_heat_generator"),
            0x01: delayed_translate("optional_backup_heater"),
            0x02: delayed_translate("wez_for_hot_water_and_heating"),
            0x03: delayed_translate("wez1_for_hot_water_wez2_for_heating")
        }
    },
    {
        "type": "select",
        "name": "ch_support" ,
        "icon": ICON_SUN_SNOWFLAKE_VARIANT,
        "command": "31 00 FA 06 6C 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: delayed_translate("off"),
            0x01: delayed_translate("on")
        }
    },
    {
        "type": "number",
        "name": "power_dhw",
        "device_class": DEVICE_CLASS_POWER,
        "unit_of_measurement": UNIT_KILOWATT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-left",
        "min_value": 1,
        "max_value": 40,
        "step": 1,
        "command": "31 00 FA 06 68 00 00",
        "handle_lambda": """
            return ((data[5] << 8) | data[6]) / 0x64;
        """,
        "set_lambda": """
            const uint16_t u16val = value * 0x64;
            data[5] = (u16val >> 8) & 0xFF;
            data[6] = u16val & 0xFF;
        """,
        "map": {
            3: "3 kW",
            6: "6 kW",
            9: "9 kW"
        }
    },
    {
        "type": "number",
        "name": "power_ehs_1",
        "device_class": DEVICE_CLASS_POWER,
        "unit_of_measurement": UNIT_KILOWATT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-left",
        "min_value": 1,
        "max_value": 40,
        "step": 1,
        "command": "31 00 FA 06 69 00 00",
        "handle_lambda": """
            return ((data[5] << 8) | data[6]) / 0x64;
        """,
        "set_lambda": """
            const uint16_t u16val = value * 0x64;
            data[5] = (u16val >> 8) & 0xFF;
            data[6] = u16val & 0xFF;
        """
    },
    {
        "type": "number",
        "name": "power_ehs_2",
        "device_class": DEVICE_CLASS_POWER,
        "unit_of_measurement": UNIT_KILOWATT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-left",
        "min_value": 1,
        "max_value": 40,
        "step": 1,
        "command": "31 00 FA 06 6A 00 00",
       "handle_lambda": """
            return ((data[5] << 8) | data[6]) / 0x64;
        """,
        "set_lambda": """
            const uint16_t u16val = value * 0x64;
            data[5] = (u16val >> 8) & 0xFF;
            data[6] = u16val & 0xFF;
        """
    },
    {
        "type": "number",
        "name": "power_biv",
        "device_class": DEVICE_CLASS_POWER,
        "unit_of_measurement": UNIT_KILOWATT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:waves-arrow-left",
        "min_value": 3,
        "max_value": 40,
        "step": 1,
        "command": "31 00 FA 06 6B 00 00",
       "handle_lambda": """
            return ((data[5] << 8) | data[6]) / 0x64;
        """,
        "set_lambda": """
            const uint16_t u16val = value * 0x64;
            data[5] = (u16val >> 8) & 0xFF;
            data[6] = u16val & 0xFF;
        """
    },
    {
        "type": "select",
        "name": "electric_heater",
        "device_class": DEVICE_CLASS_POWER,
        "unit_of_measurement": UNIT_KILOWATT,
        "accuracy_decimals": 0,
        "state_class": STATE_CLASS_MEASUREMENT,
        "icon": "mdi:induction",
        "command": "31 00 FA 0A 20 00 00",
        "data_offset": 5,
        "data_size": 2,
        "map": {
            0x00: delayed_translate("off"),
            0x03: "3 kW",
            0x06: "6 kW",
            0x09: "9 kW"
        },
        "handle_lambda": """
            return
                bool(data[5] & 0b00001000) * 3 +
                bool(data[5] & 0b00000100) * 3 +
                bool(data[5] & 0b00000010) * 3;
        """,
        "set_lambda": """
            data[5] = 0b00000001;
            if (value >= 3) data[5] |= 0b00001000;
            if (value >= 6) data[5] |= 0b00000100;
            if (value >= 9) data[5] |= 0b00000010;
        """
    },
    {
        "type": "text_sensor",
        "name": "ext",
        "accuracy_decimals": 0,
        "icon": "mdi:transmission-tower-import",
        "command": "31 00 FA C0 F8 00 00",
        "data_offset": 6,
        "data_size": 1,
        "map": {
            0x00: "---",
            0x03: delayed_translate("sgn_normal_mode"),
            0x04: delayed_translate("sg1_hot_water_and_heating_off"),
            0x05: delayed_translate("sg2_hot_water_and_heating_plus_5c"),
            0x06: delayed_translate("sg3_hot_water_70c")
        }
    },

    {
        "type": "number",
        "name": "max_heating_temperature",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_CELSIUS,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 5,
        "max_value": 85,
        "step": 1,
        "command": "31 00 FA 06 6E",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
#    {
#        "type": "number",
#        "name": "bivalent_temperature",
#        "device_class": DEVICE_CLASS_TEMPERATURE,
#        "unit_of_measurement": UNIT_CELSIUS,
#        "accuracy_decimals": 1,
#        "state_class": STATE_CLASS_MEASUREMENT,
#        "min_value": -15,
#        "max_value": 35,
#        "step": 1,
#        "can_id": 0x500,
#        "command": "31 00 FA 06 D4",
#        "data_offset": 5,
#        "data_size": 2,
#        "divider": 10.0
#    },
    {
        "type": "number",
        "name": "supply_temperature_adjustment_heating",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_KELVIN,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 0,
        "max_value": 50,
        "step": 1,
        "command": "31 00 FA 06 A0",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "number",
        "name": "supply_temperature_adjustment_cooling",
        "device_class": DEVICE_CLASS_TEMPERATURE,
        "unit_of_measurement": UNIT_KELVIN,
        "accuracy_decimals": 1,
        "state_class": STATE_CLASS_MEASUREMENT,
        "min_value": 0,
        "max_value": 50,
        "step": 1,
        "command": "31 00 FA 06 A1",
        "data_offset": 5,
        "data_size": 2,
        "divider": 10.0
    },
    {
        "type": "select",
        "name": "optimized_defrosting",
        "icon": "mdi:snowflake-melt",
        "map": {
            0x00: delayed_translate("off"),
            0x01: delayed_translate("on")
        }
    }
]

CODEOWNERS = ["@wrfz"]
DEPENDENCIES = []
AUTO_LOAD = ['binary_sensor', 'button', 'number', 'sensor', 'select', 'text', 'text_sensor']

CONF_CAN_ID = "canbus_id"
CONF_UPDATE_INTERVAL = "update_interval"
CONF_MAX_SPREAD_TVBH_TV = "max_spread_tvbh_tv"
CONF_MAX_SPREAD_TVBH_TR = "max_spread_tvbh_tr"
CONF_LOG_FILTER_TEXT = "log_filter"
CONF_CUSTOM_REQUEST_TEXT = "custom_request"
CONF_ENTITIES = "entities"
CONF_SELECT_OPTIONS = "options"
CONF_PROJECT_GIT_HASH = "project_git_hash"

########## Sensors ##########

CONF_THERMAL_POWER = "thermal_power" # Thermische Leistung

CONF_DUMP = "dump"
CONF_DHW_RUN = "dhw_run"

DEFAULT_UPDATE_INTERVAL = 30 # seconds
DEFAULT_MAX_SPREAD_TVBH_TV = 3.0
DEFAULT_MAX_SPREAD_TVBH_TR = 3.0

entity_schemas = {}

for sensor_conf in sensor_configuration:
    name = sensor_conf.get("name")
    icon = sensor_conf.get("icon", sensor._UNDEF)
    divider = sensor_conf.get("divider", 1.0)

    match sensor_conf.get("type"):
        case "sensor":
            entity_schemas.update({
                cv.Optional(name): sensor.sensor_schema(
                    CanSensor,
                    device_class=(sensor_conf.get("device_class") if sensor_conf.get("device_class") != None else sensor._UNDEF),
                    unit_of_measurement=sensor_conf.get("unit_of_measurement"),
                    accuracy_decimals=sensor_conf.get("accuracy_decimals"),
                    state_class=sensor_conf.get("state_class"),
                    icon=sensor_conf.get("icon", sensor._UNDEF)
                ).extend({cv.Optional(CONF_UPDATE_INTERVAL): cv.uint16_t}),
            })
        case "text_sensor":
            entity_schemas.update({
                cv.Optional(name): text_sensor.text_sensor_schema(
                    CanTextSensor,
                    icon=sensor_conf.get("icon", text_sensor._UNDEF)
                ).extend({cv.Optional(CONF_UPDATE_INTERVAL): cv.uint16_t}),
            })
        case "binary_sensor":
            entity_schemas.update({
                cv.Optional(name): binary_sensor.binary_sensor_schema(
                    CanBinarySensor,
                    icon=sensor_conf.get("icon", binary_sensor._UNDEF)
                ).extend({cv.Optional(CONF_UPDATE_INTERVAL): cv.uint16_t}),
            })
        case "select":
            entity_schemas.update({
                cv.Optional(name): select.select_schema(
                    CanSelect,
                    entity_category=ENTITY_CATEGORY_CONFIG,
                    icon=sensor_conf.get("icon", select._UNDEF)
                ).extend({cv.Optional(CONF_UPDATE_INTERVAL): cv.uint16_t}),
            })
        case "number":
            select_options_schema = cv.Optional(CONF_SELECT_OPTIONS) if "map" in sensor_conf else cv.Required(CONF_SELECT_OPTIONS)
            entity_schemas.update({
                cv.Optional(name): cv.typed_schema(
                    {
                        "number": number.number_schema(
                            CanNumber,
                            entity_category=ENTITY_CATEGORY_CONFIG,
                            icon=sensor_conf.get("icon", number._UNDEF)
                        ).extend({
                            cv.Optional(CONF_UPDATE_INTERVAL): cv.uint16_t,
                            cv.Optional(CONF_MODE, default="BOX"): cv.enum(number.NUMBER_MODES, upper=True)
                        }),
                        "select": select.select_schema(
                            CanSelect,
                            entity_category=ENTITY_CATEGORY_CONFIG,
                            icon=sensor_conf.get("icon", select._UNDEF)
                        ).extend({
                            cv.Optional(CONF_UPDATE_INTERVAL): cv.uint16_t,
                            select_options_schema: cv.Schema({
                                cv.float_range(
                                    min=sensor_conf.get("min_value"),
                                    max=sensor_conf.get("max_value")
                                ): cv.string
                            })
                        }),
                    },
                    default_type="number"
                )
            })

entity_schemas.update({
    ########## Sensors ##########

    cv.Optional(CONF_THERMAL_POWER): sensor.sensor_schema(
        device_class=DEVICE_CLASS_POWER,
        unit_of_measurement=UNIT_KILOWATT,
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT
    ).extend(),

    ########## Buttons ##########

    cv.Optional(CONF_DHW_RUN): button.button_schema(
        DHWRunButton,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon=ICON_SUN_SNOWFLAKE_VARIANT
    ).extend(),
})

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(DaikinRotexCanComponent),
        cv.Required(CONF_CAN_ID): cv.use_id(CanbusComponent),
        cv.Optional(CONF_UPDATE_INTERVAL, default=DEFAULT_UPDATE_INTERVAL): cv.uint16_t,
        cv.Optional(CONF_MAX_SPREAD_TVBH_TV, default=DEFAULT_MAX_SPREAD_TVBH_TV): cv.float_,
        cv.Optional(CONF_MAX_SPREAD_TVBH_TR, default=DEFAULT_MAX_SPREAD_TVBH_TR): cv.float_,
        cv.Optional(CONF_LANGUAGE, default="de"): cv.string,

        ########## Texts ##########

        cv.Optional(CONF_LOG_FILTER_TEXT): text.TEXT_SCHEMA.extend(
            {
                cv.GenerateID(): cv.declare_id(LogFilterText),
                cv.Optional(CONF_MODE, default="TEXT"): cv.enum(text.TEXT_MODES, upper=True),
            }
        ),
        cv.Optional(CONF_CUSTOM_REQUEST_TEXT): text.TEXT_SCHEMA.extend(
            {
                cv.GenerateID(): cv.declare_id(CustomRequestText),
                cv.Optional(CONF_MODE, default="TEXT"): cv.enum(text.TEXT_MODES, upper=True),
            }
        ),
        cv.Required(CONF_PROJECT_GIT_HASH): text_sensor.text_sensor_schema(
            icon="mdi:git",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),

        ########## Buttons ##########

        cv.Optional(CONF_DUMP): button.button_schema(
            DumpButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
            icon=ICON_SUN_SNOWFLAKE_VARIANT
        ).extend(),

        cv.Required(CONF_ENTITIES): cv.Schema(
            entity_schemas
        ),
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):

    if CONF_LANGUAGE in config:
        lang = config[CONF_LANGUAGE]
        set_language(lang)

    global_ns = MockObj("", "")
    std_array_u8_7_const_ref = std_ns.class_("array<uint8_t, 7> const&")
    std_array_u8_7_ref = std_ns.class_("array<uint8_t, 7>&")

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    if CONF_CAN_ID in config:
        cg.add_define("USE_CANBUS")
        canbus = await cg.get_variable(config[CONF_CAN_ID])
        cg.add(var.set_canbus(canbus))

    cg.add(var.set_max_spread(config[CONF_MAX_SPREAD_TVBH_TV], config[CONF_MAX_SPREAD_TVBH_TR]))

    # Write cpp translation file
    write_cpp_file(os.path.dirname(__file__), current_language)

    ########## Texts ##########

    if text_conf := config.get(CONF_LOG_FILTER_TEXT):
        await text.new_text(text_conf)

    if text_conf := config.get(CONF_CUSTOM_REQUEST_TEXT):
        t = await text.new_text(text_conf)
        await cg.register_parented(t, var)

    ########## Text Sensors ##########

    if text_conf := config.get(CONF_PROJECT_GIT_HASH):
        t = await text_sensor.new_text_sensor(text_conf)
        cg.add(var.set_project_git_hash(t, git_hash))

    ########## Buttons ##########

    if button_conf := config.get(CONF_DUMP):
        but = await button.new_button(button_conf)
        await cg.register_parented(but, var)

    if entities := config.get(CONF_ENTITIES):
        for sens_conf in sensor_configuration:
            if yaml_sensor_conf := entities.get(sens_conf.get("name")):
                entity = None
                divider = sens_conf.get("divider", 1.0)

                # translate both map and name (if auto)
                mapping = apply_translation_to_mapping(sens_conf.get("map", {}))
                apply_translation_to_entityname(yaml_sensor_conf,sens_conf.get("name"))

                if yaml_sensor_conf.get("type") == "select" and "options" in yaml_sensor_conf:
                    mapping = yaml_sensor_conf.get("options")
                str_map = "|".join([f"0x{int(key * divider) & 0xFFFF :02X}:{value}" for key, value in mapping.items()])

                match sens_conf.get("type"):
                    case "sensor":
                        entity = await sensor.new_sensor(yaml_sensor_conf)
                        cg.add(entity.set_range(sens_conf.get("range", [0, 0])))
                    case "text_sensor":
                        entity = await text_sensor.new_text_sensor(yaml_sensor_conf)
                        cg.add(entity.set_map(str_map))
                    case "binary_sensor":
                        entity = await binary_sensor.new_binary_sensor(yaml_sensor_conf)
                    case "select":
                        entity = await select.new_select(yaml_sensor_conf, options = list(mapping.values()))
                        cg.add(entity.set_map(str_map))
                        await cg.register_parented(entity, var)
                    case "number":
                        if "min_value" not in sens_conf:
                            raise Exception("min_value is required for number: " + sens_conf.get("name"))
                        if "max_value" not in sens_conf:
                            raise Exception("max_value is required for number: " + sens_conf.get("name"))
                        if "step" not in sens_conf:
                            raise Exception("step is required for number: " + sens_conf.get("name"))

                        match yaml_sensor_conf.get("type"):
                            case "number":
                                entity = await number.new_number(
                                    yaml_sensor_conf,
                                    min_value=sens_conf.get("min_value"),
                                    max_value=sens_conf.get("max_value"),
                                    step=sens_conf.get("step")
                                )
                            case "select":
                                entity = await select.new_select(yaml_sensor_conf, options = list(mapping.values()))
                                cg.add(entity.set_map(str_map))

                        await cg.register_parented(entity, var)
                    case _:
                        raise Exception("Unknown type: " + sens_conf.get("type"))

                update_interval = yaml_sensor_conf.get(CONF_UPDATE_INTERVAL, -1)
                if update_interval < 0:
                    update_interval = config[CONF_UPDATE_INTERVAL]

                async def handle_lambda():
                    lamb = str(sens_conf.get("handle_lambda")) if "handle_lambda" in sens_conf else "return 0;"
                    return await cg.process_lambda(
                        Lambda(lamb),
                        [(std_array_u8_7_const_ref, "data")],
                        return_type=cg.uint16,
                    )

                async def set_lambda():
                    lamb = str(sens_conf.get("set_lambda")) if "set_lambda" in sens_conf else ""
                    return await cg.process_lambda(
                        Lambda(lamb),
                        [(std_array_u8_7_ref, "data"), (cg.uint16, "value")],
                        return_type=cg.void,
                    )

                cg.add(entity.set_entity(sens_conf.get("name"), [
                    entity,
                    sens_conf.get("name"), # Entity id
                    sens_conf.get("can_id", 0x180),
                    sens_conf.get("command", ""),
                    sens_conf.get("data_offset", 5),
                    sens_conf.get("data_size", 1),
                    divider,
                    sens_conf.get("signed", False),
                    sens_conf.get("update_entity", ""),
                    update_interval,
                    await handle_lambda(),
                    await set_lambda(),
                    "handle_lambda" in sens_conf,
                    "set_lambda" in sens_conf
                ]))
                cg.add(var.add_entity(entity));

        ########## Sensors ##########

        if yaml_sensor_conf := entities.get(CONF_THERMAL_POWER):
            apply_translation_to_entityname(yaml_sensor_conf, CONF_THERMAL_POWER)
            sens = await sensor.new_sensor(yaml_sensor_conf)
            cg.add(var.set_thermal_power_sensor(sens))

        ########## Buttons ##########

        if button_conf := entities.get(CONF_DHW_RUN):
            but = await button.new_button(button_conf)
            await cg.register_parented(but, var)
