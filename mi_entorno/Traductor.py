import subprocess
import sys
import os

def mostrar_menu():
    print("="*60)
    print("🎮 CONTROL TRADUCTOR ESP32-CAM")
    print("="*60)
    print("1. 🖥️  Modo Local (Cámara PC)")
    print("2. 📡 Modo Servidor (ESP32-CAM)")
    print("3. ❌ Salir")
    
    opcion = input("\nSelecciona opción (1-3): ").strip()
    
    if opcion == '1':
        print("\n🔄 Iniciando modo local...")
        os.system("python Treaduccion.py")
    elif opcion == '2':
        print("\n🔄 Iniciando servidor para ESP32-CAM...")
        os.system("python servidor_traductor.py")
    elif opcion == '3':
        print("👋 ¡Hasta luego!")
        sys.exit(0)
    else:
        print("❌ Opción no válida")

if __name__ == "__main__":
    while True:
        mostrar_menu()