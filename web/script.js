//ICONS


/*
<a href="https://www.flaticon.com/free-icons/navigation" title="navigation icons">Navigation icons created by HideMaru - Flaticon</a>

*/


// Configuración inicial del mapa
const map = L.map('map').setView([27.9598, -15.5621], 11); // Coordenadas iniciales y zoom
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
  attribution: 'Map data © <a href="https://www.openstreetmap.org/">OpenStreetMap</a> contributors',
}).addTo(map);



// Lista de Balizas
const ListBalizas = [] 

// GPSInfo
const GPSList = [{name: "GPS2" ,id:23,lat:-1,lon:-1,marker: L.marker([-1,-1])},{name: "GPS1" , id:22,lat:-1,lon:-1,marker: L.marker([-1,-1])},{name: "GPS3" ,id:32,lat:-1,lon:-1,marker: L.marker([-1,-1])}]

var customIcon1 = L.icon({
  iconUrl: 'https://github.com/OscarMunozHidalgo/GPS_Error_Correction/blob/main/web/gpsBLUE.png?raw=true', // Path to the icon
  iconSize: [45, 45], // Size of the icon
  iconAnchor: [45, 45], // Point of the icon that corresponds to marker's location
  popupAnchor: [-23, -40], // Point from which the popup should open relative to the iconAnchor
});

var customIcon2 = L.icon({
  iconUrl: 'https://github.com/OscarMunozHidalgo/GPS_Error_Correction/blob/main/web/gpsGREEN.png?raw=true', // Path to the icon
  iconSize: [45, 45], // Size of the icon
  iconAnchor: [45, 45], // Point of the icon that corresponds to marker's location
  popupAnchor: [-23, -40], // Point from which the popup should open relative to the iconAnchor
});

var customIcon3 = L.icon({
  iconUrl: 'https://github.com/OscarMunozHidalgo/GPS_Error_Correction/blob/main/web/gpsPURPLE.png?raw=true', // Path to the icon
  iconSize: [45, 45], // Size of the icon
  iconAnchor: [45, 45], // Point of the icon that corresponds to marker's location
  popupAnchor: [-23, -40], // Point from which the popup should open relative to the iconAnchor
});

GPSList[0].marker.setIcon(customIcon1)
GPSList[1].marker.setIcon(customIcon2)
GPSList[2].marker.setIcon(customIcon3)

function createBaliza(id,lon,lat){
  const balizafoo = 
  {
    id:id,
    lon:lon,
    lat:lat,
    marker: L.marker([lon,lat]).addTo(map)
  }
  ListBalizas.push(balizafoo)
  return balizafoo
}

// Crear un marcador
function CreateMarker (lat,lon){
  const marker = L.marker(lat,lon)
  return marker
}


// Configuración del cliente MQTT
const mqttConfig = {
  host: '5.250.190.40', // Cambia si usas un broker diferente
  port: 9001,               // Puerto WebSocket del broker
  topic: 'gps/tracker',     // Tema MQTT
};


// Conectar al broker MQTT
const client = new Paho.MQTT.Client(mqttConfig.host, mqttConfig.port, "webClient_" + Math.random());
client.onMessageArrived = onMessageArrived;

// Función para manejar mensajes entrantes
function onMessageArrived(message) {
  console.log(`Mensaje recibido: ${message.payloadString}`);
  
  try {
    const data = JSON.parse(message.payloadString); // Esperamos un JSON con lat y lon
    if(data.type == "GPS"){
      const {id,lon,lat} = data.description
      var element = GPSList.find( a => a.id == id)
      element.lat = lat/10000000
      element.lon = lon/10000000
      element.marker.setLatLng([element.lat,element.lon = lon/10000000])
      element.marker.addTo(map)
      element.marker.bindPopup("Nombre: " + element.name + " Cordenadas---> lat:" + element.lat + " lon:" + element.lon)
      console.log(GPSList[0])
    }else{
      const {id,lon,lat} = data.description
      if(balizafoo.length == 0){
        baliza = createBaliza(id,lon,lat)
        baliza.marker.setLatLng([lat,lon])
        baliza.marker.addTo(map)
      }
      else{
        balizafoo[0].lat = lat
        balizafoo[0].lon = lon
        balizafoo[0].marker.setLatLng([lat,lon])
      }
      
    }
  } catch (error) {
    console.error('Error procesando mensaje MQTT:', error);
  }
}

// Función para manejar la conexión
client.onConnectionLost = function (responseObject) {
  console.error('Conexión perdida:', responseObject.errorMessage);
};

// Conexión al broker
client.connect({
  userName: "IC",
  password:"123456", 
  onSuccess: () => {
    console.log('Conectado al broker MQTT');
    client.subscribe(mqttConfig.topic); // Suscribir al tema
  },
  onFailure: (error) => {
    console.error('Error al conectar:', error.errorMessage);
  },
});
