// META: script=/resources/testharness.js
// META: script=/resources/testharnessreport.js
// META: script=/gen/layout_test_data/mojo/public/js/mojo_bindings.js
// META: script=/gen/mojo/public/mojom/base/unguessable_token.mojom.js
// META: script=/gen/third_party/blink/public/mojom/serial/serial.mojom.js
// META: script=resources/common.js
// META: script=resources/automation.js

serial_test(async (t, fake) => {
  const {port, fakePort} = await getFakeSerialPort(fake);
  await port.open({baudRate: 9600, bufferSize: 64});

  const reader = port.readable.getReader();
  const readPromise = reader.read();
  await reader.cancel();
  const {value, done} = await readPromise;
  assert_true(done);
  assert_equals(undefined, value);

  await port.close();
}, 'Can cancel while reading');

serial_test(async (t, fake) => {
  const {port, fakePort} = await getFakeSerialPort(fake);
  await port.open({baudRate: 9600, bufferSize: 64});

  const reader = port.readable.getReader();

  await fakePort.writable();
  const data = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8]);
  await fakePort.write(data);

  await reader.cancel();
  await port.close();
}, 'Cancel discards a small amount of data waiting to be read');

serial_test(async (t, fake) => {
  const {port, fakePort} = await getFakeSerialPort(fake);
  // Select a buffer size smaller than the amount of data transferred.
  await port.open({baudRate: 9600, bufferSize: 64});

  const reader = port.readable.getReader();

  await fakePort.writable();
  const data = new Uint8Array(1024);
  // Writing will fail because there was more data to send than could fit in the
  // buffer and none of it was read.
  const writePromise =
      promise_rejects_dom(t, 'InvalidStateError', fakePort.write(data));

  await reader.cancel();
  await writePromise;

  await port.close();
}, 'Cancel discards a large amount of data waiting to be read');
