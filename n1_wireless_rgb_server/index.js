const dgram = require('dgram');

const server = dgram.createSocket('udp4');

const UDP_PORT = 44444;
const CLIENT_ADDRESS = '192.168.1.103';	

const COLOR_DEPTH = 16;

const CHUNK_SIZE = 1024; // Safe UDP packet size

const INTERVAL = 1000 / 20;


// Create a buffer of 32x32 pixels with 16-bit color (2 bytes per pixel)
const pixels = new Uint8Array(32 * 32 * (COLOR_DEPTH / 8));


// Function to send pixels in chunks
function sendPixels(pixels, clientAddress = CLIENT_ADDRESS, clientPort = UDP_PORT, chunkSize = CHUNK_SIZE) {
    
    const totalChunks = Math.ceil(pixels.length / chunkSize);

    for (let i = 0; i < totalChunks; i++) {
        const start = i * chunkSize;
        const end = Math.min(start + chunkSize, pixels.length);
        const chunk = pixels.slice(start, end);

        // Prepend chunk index and total chunks to the data
        const chunkHeader = new Uint8Array([i, totalChunks]);
        const chunkWithHeader = new Uint8Array(chunkHeader.length + chunk.length);
        chunkWithHeader.set(chunkHeader);
        chunkWithHeader.set(chunk, chunkHeader.length);

        server.send(chunkWithHeader, clientPort, clientAddress, (err) => {
            if (err) {
                console.log(`Error sending chunk ${i}:`, err);
            } else {
                console.log(`Chunk ${i + 1}/${totalChunks} sent to ${clientAddress}:${clientPort}`);
            }
        });
    }
}

server.bind(UDP_PORT);

let frame = 0;
setInterval(() => {

	// for (let i = 0; i < 32 * 32; i++) {
	// 	const r = (i % 32) * 8;
	// 	const g = Math.floor(i / 32) * 8;
	// 	const b = 128;
	
	// 	const rgb565 = ((r & 0xF8) << 8) |
	// 				   ((g & 0xFC) << 3) |
	// 				   (b >> 3);
	
	// 	pixels[i * 2] = (rgb565 >> 8) & 0xFF;
	// 	pixels[i * 2 + 1] = rgb565 & 0xFF;
	// }
	// pixels[0] = frame % 2 * 100;
	// frame++;

	if (frame % pixels.length === 0) {
		for (let i = 0; i < pixels.length; i++) {
			pixels[i] = 0
		}
	} 

	pixels[frame % pixels.length] = Math.floor(Math.random() * 100);;

	frame++


    sendPixels(pixels);

}, INTERVAL);