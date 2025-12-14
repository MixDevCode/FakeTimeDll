# FakeTimeDLL

FakeTime DLL es un componente de bajo nivel desarrollado en C++ para sistemas Windows, diseñado para **interceptar y controlar las funciones de obtención de fecha y hora del sistema dentro de un proceso específico**.  
Este proyecto forma parte de una solución mayor orientada a la simulación y manipulación del tiempo de ejecución de aplicaciones.

Esta DLL fue realizada para trabajar en conjunto con:  
https://github.com/MixDevCode/RunAsDate

## Descripción general

La biblioteca se implementa como una DLL que utiliza **API hooking** mediante la librería **MinHook** para interceptar llamadas a funciones críticas de tiempo provistas por `kernel32.dll`, entre ellas:

- `GetSystemTime`
- `GetLocalTime`
- `GetSystemTimeAsFileTime`

A partir de una configuración centralizada, la DLL puede alterar el valor de tiempo retornado a la aplicación, sin modificar la fecha u hora real del sistema operativo.

## Arquitectura y funcionamiento

Al momento de ser cargada en el proceso objetivo, la DLL:

1. Inicializa MinHook.
2. Instala hooks sobre las funciones de tiempo del sistema.
3. Redirige las llamadas a implementaciones personalizadas que determinan el tiempo efectivo a devolver según el modo configurado.

El comportamiento es controlado mediante variables globales exportadas:

- `g_TimeMode`: define si se devuelve tiempo real o simulado.
- `g_FakeTime`: expone el último valor de tiempo simulado aplicado.
- `g_ConfigData`: estructura de configuración que define el modo de simulación.

La lógica interna permite:

- Devolver el tiempo real del sistema.
- Simular un tiempo base fijo.
- Avanzar el tiempo de manera progresiva utilizando contadores de alto rendimiento.
- Retornar automáticamente al tiempo real luego de un intervalo configurable.

Todas las operaciones se realizan **exclusivamente en el contexto del proceso donde se carga la DLL**, sin afectar el reloj del sistema ni otros procesos en ejecución.

## Casos de uso previstos

- Pruebas y validación de software dependiente de fecha y hora.
- Simulación de vencimientos temporales.
- Ejecución controlada de aplicaciones heredadas.
- Análisis y depuración de comportamientos relacionados con el tiempo.

## Tecnologías utilizadas

- C++
- Windows API
- MinHook
- Técnicas de API Hooking mediante DLL

## Alcance del proyecto

Este repositorio contiene únicamente la implementación de la DLL.  
No incluye herramientas de inyección ni interfaces de usuario, las cuales forman parte del proyecto principal que la integra.

## Consideraciones

Este componente debe utilizarse únicamente en entornos controlados y con fines legítimos, como pruebas, análisis o desarrollo.  
Es responsabilidad del usuario final asegurar el cumplimiento de las licencias y normativas aplicables.

## Licencia

Uso educativo y experimental. Revisar las licencias de las dependencias utilizadas antes de su distribución o integración en proyectos comerciales.
