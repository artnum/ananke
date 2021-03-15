let enc = new AKEncoder()
let socket
function sopen(event) {
    socket = new WebSocket('ws://localhost:9991', 'ananke')
    socket.onopen = function (event) {
        console.log(event)
    }
    socket.onmessage = function (event) {
        console.log(event)
        document.getElementById('received').value += event.data + "\n"
        let json = JSON.parse(event.data)
        switch(json.operation) {
            case 'connected':

                break;
            case 'ping':
                socket.send(enc.encodeObject('', {'operation': 'pong', 'count': json.count, 'opid': Math.random().toString(16).substr(2, 16)}))
                break;
        }
    }
}

function send (event) {
    const json = JSON.parse(document.getElementById('send').value)
    json.opid = Math.random().toString(16).substr(2, 16);
    const value = enc.encodeObject('', json)
    
    socket.send(value)
}

function close (event) {
    socket.close()
}