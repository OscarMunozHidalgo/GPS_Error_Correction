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
const GPSList = [{id:23,lat:-1,lon:-1,marker: L.marker([-1,-1])},{id:22,lat:-1,lon:-1,marker: L.marker([-1,-1])},{id:3,lat:-1,lon:-1,marker: L.marker([-1,-1])}]

var customIcon = L.icon({
  iconUrl: 'https://e7.pngegg.com/pngimages/839/441/png-clipart-gps-icon-gps-icon-thumbnail.png', // Path to the icon
  iconSize: [45, 45], // Size of the icon
  iconAnchor: [45, 45], // Point of the icon that corresponds to marker's location
  popupAnchor: [-23, -40], // Point from which the popup should open relative to the iconAnchor
});

GPSList[0].marker.setIcon(customIcon)

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
      GPSList[0].id = id
      GPSList[0].lat = lat/10000000
      GPSList[0].lon = lon/10000000
      GPSList[0].marker.setLatLng([GPSList[0].lat,GPSList[0].lon = lon/10000000])
      GPSList[0].marker.addTo(map)
      GPSList[0].marker.bindPopup("Cordenadas---> lat:" + GPSList[0].lat + " lon:" + GPSList[0].lon)
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
