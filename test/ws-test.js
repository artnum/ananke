let enc = new AKEncoder()
let socket
function sopen(event) {
    socket = new WebSocket('ws://localhost:9991', 'ananke')
    socket.onopen = function (event) {
        console.log(event)
    }
    socket.onmessage = function (event) {
        console.log(event)
        document.getElementById('received').value = event.data
        let json = JSON.parse(event.data)
        switch(json.operation) {
            case 'connected':

                break;
            case 'ping':
                socket.send(enc.encodeObject('', {'operation': 'pong', 'count': json.count}))
                break;
        }
    }
}

function send (event) {
    const value = enc.encodeObject('', JSON.parse(document.getElementById('send').value))
    socket.send(value)
}

function close (event) {
    socket.close()
}