# Bluetooth Audio Receiver - A2DP Sink Refactoring Plan

## 🎯 **Objective**
Refactor the application from a Bluetooth client (connecting to external devices) to a Bluetooth A2DP sink server (receiving connections from phones/devices) with audio mixing capabilities.

## 📋 **Current vs. Target Architecture**

### **Current Architecture (Client Mode)**
```
PC (Client) ──connects to──> Bluetooth Speakers/Headphones
PC ──sends audio──> External Device ──outputs──> Audio
```

### **Target Architecture (Sink/Server Mode)**  
```
Phone/Device ──connects to──> PC (A2DP Sink)
Phone ──streams audio──> PC ──mixes──> PC Speakers + Phone Audio
```

## 🔧 **Key Refactoring Components**

### 1. **Bluetooth A2DP Sink Service**
- **Current**: `BluetoothManager` discovers and connects to devices
- **Target**: `BluetoothSinkService` advertises PC as A2DP sink and accepts connections
- **Changes**:
  - Bluetooth service advertising (make PC discoverable as audio sink)
  - Incoming connection handling
  - A2DP sink profile implementation
  - Multiple device connection management

### 2. **Audio Stream Management**  
- **Current**: `AudioManager` controls output to external devices
- **Target**: `AudioStreamManager` receives, decodes, and mixes audio streams
- **Changes**:
  - Bluetooth audio stream reception
  - Audio format conversion (various codecs to PCM)
  - Real-time audio mixing with PC audio
  - Low-latency audio pipeline

### 3. **Audio Mixing Engine**
- **New Component**: `AudioMixer` 
- **Functionality**:
  - Mix incoming Bluetooth audio with PC system audio
  - Volume control for each audio source
  - Real-time audio processing
  - Output to default PC audio device

### 4. **Connection Management**
- **Current**: Track outgoing connections to discovered devices
- **Target**: Track incoming connections from source devices
- **Changes**:
  - Connection state management for multiple sources
  - Device authentication and pairing
  - Connection quality monitoring
  - Auto-reconnection handling

### 5. **User Interface Updates**
- **Current**: Shows discovered devices with connect/disconnect options
- **Target**: Shows connected audio sources with mixing controls
- **Changes**:
  - Connected devices list (instead of discovered devices)
  - Per-device volume controls and mute options
  - Audio mixing status and controls
  - Connection quality indicators

## 🏗️ **Implementation Plan**

### **Phase 1: Core Architecture Refactoring**
1. **Rename and refactor core classes**:
   - `BluetoothManager` → `BluetoothSinkService`
   - Add `AudioMixer` component
   - Add `BluetoothAudioStream` class

2. **Update data models**:
   - `BluetoothDevice` → `ConnectedAudioSource`
   - Add audio stream properties
   - Add mixing state information

### **Phase 2: Bluetooth A2DP Sink Implementation**
1. **Service Advertising**:
   - Implement Bluetooth service discovery advertising
   - Make PC visible as "Audio Sink" device
   - Handle incoming connection requests

2. **A2DP Sink Profile**:
   - Implement A2DP sink role
   - Handle SBC/AAC audio codec negotiation
   - Manage audio stream establishment

### **Phase 3: Audio Processing Pipeline**
1. **Stream Reception**:
   - Receive Bluetooth audio packets
   - Decode audio streams (SBC/AAC → PCM)
   - Buffer management for smooth playback

2. **Audio Mixing**:
   - Mix multiple incoming audio streams
   - Combine with existing PC audio
   - Volume normalization and control

### **Phase 4: UI and Management**
1. **Update UI Components**:
   - Show connected audio sources
   - Individual volume controls
   - Mix status and quality indicators

2. **Configuration Management**:
   - Audio mixing preferences
   - Device connection policies
   - Audio quality settings

## 📁 **File Structure Changes**

### **New Files**:
```
src/Core/
├── BluetoothSinkService.h/cpp (renamed from BluetoothManager)
├── AudioMixer.h/cpp (new)
├── BluetoothAudioStream.h/cpp (new)
└── AudioStreamManager.h/cpp (enhanced AudioManager)

src/Models/
├── ConnectedAudioSource.h (renamed from BluetoothDevice)
├── AudioStreamInfo.h (new)
└── MixingState.h (new)

src/Platform/
└── BluetoothSinkAPI.h/cpp (enhanced WindowsAPI)
```

### **Modified Files**:
- `Application.h/cpp` - Update for new architecture
- `WindowsAPI.h/cpp` - Add A2DP sink functions
- UI rendering components - Show connected sources

## 🔌 **Technical Requirements**

### **Windows API Integration**:
- **Bluetooth APIs**: Use Windows Bluetooth APIs for A2DP sink
- **WASAPI**: Enhanced audio mixing via Windows Audio Session API
- **Audio Formats**: Support SBC, AAC codec decoding
- **Real-time Audio**: Low-latency audio pipeline

### **Audio Pipeline**:
```
Bluetooth Stream → Decoder → Buffer → Mixer → WASAPI → Speakers
                                ↑
PC System Audio ──────────────────┘
```

### **Connection Flow**:
```
1. PC advertises as A2DP sink
2. Phone discovers PC as audio device  
3. Phone connects and pairs
4. Audio stream established
5. PC receives and mixes audio
6. Combined output to PC speakers
```

## 🎯 **Success Criteria**

✅ **PC is discoverable as Bluetooth audio device**  
✅ **Phones can connect and stream audio to PC**  
✅ **Phone audio mixes seamlessly with PC audio**  
✅ **Multiple devices can connect simultaneously**  
✅ **Individual volume control for each source**  
✅ **Low audio latency and good quality**  
✅ **Stable connections with auto-reconnect**

## 🚀 **Implementation Priority**

1. **High Priority**: Basic A2DP sink functionality
2. **High Priority**: Single device audio mixing  
3. **Medium Priority**: Multiple device support
4. **Medium Priority**: Advanced audio controls
5. **Low Priority**: Audio codec optimization
6. **Low Priority**: Advanced mixing features

This refactoring will transform the app from a simple Bluetooth client into a powerful audio mixing hub that makes your PC a Bluetooth audio receiver! 
