import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

void main() {
  runApp(const MonsterEngineTestApp());
}

/// Minimal test app that demonstrates Flutter ↔ Monster Engine communication.
///
/// Shows a status dashboard with:
/// - Connection status to the engine
/// - Messages received from the C++ side
/// - Ability to send ping/pong messages via platform channels
class MonsterEngineTestApp extends StatelessWidget {
  const MonsterEngineTestApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Monster Engine Flutter Test',
      debugShowCheckedModeBanner: false,
      theme: ThemeData.dark(useMaterial3: true).copyWith(
        colorScheme: ColorScheme.dark(
          primary: const Color(0xFF64FFDA),
          secondary: const Color(0xFFFF6E40),
          surface: const Color(0xFF1A1A2E),
        ),
        scaffoldBackgroundColor: Colors.transparent,
      ),
      home: const TestDashboard(),
    );
  }
}

class TestDashboard extends StatefulWidget {
  const TestDashboard({super.key});

  @override
  State<TestDashboard> createState() => _TestDashboardState();
}

class _TestDashboardState extends State<TestDashboard> {
  static const _channel = MethodChannel('monster/test');

  String _status = 'Initializing...';
  String _engineInfo = 'Unknown';
  int _tickCount = 0;
  int _fps = 0;
  final List<String> _log = [];

  @override
  void initState() {
    super.initState();
    _setupChannel();
    _pingEngine();
  }

  void _setupChannel() {
    _channel.setMethodCallHandler((call) async {
      switch (call.method) {
        case 'engineTick':
          final args = call.arguments as Map<dynamic, dynamic>?;
          setState(() {
            _tickCount = args?['tick'] ?? _tickCount + 1;
            _fps = args?['fps'] ?? 0;
            _log.insert(0, 'Tick #$_tickCount (${_fps} fps)');
            if (_log.length > 20) _log.removeLast();
          });
          return 'ok';
        default:
          setState(() {
            _log.insert(0, 'Unknown: ${call.method}');
          });
          return null;
      }
    });
  }

  Future<void> _pingEngine() async {
    try {
      final result = await _channel.invokeMethod('ping');
      final data = result is String ? jsonDecode(result) : result;
      setState(() {
        _status = data['status'] ?? 'connected';
        _log.insert(0, 'Ping → ${data['status']}');
      });
    } catch (e) {
      setState(() {
        _status = 'No connection';
        _log.insert(0, 'Ping failed: $e');
      });
    }

    // Also request engine info.
    try {
      final result = await _channel.invokeMethod('getEngineInfo');
      final data = result is String ? jsonDecode(result) : result;
      setState(() {
        _engineInfo = '${data['name']} (${data['renderer']})';
      });
    } catch (e) {
      // Ignore — engine info is optional.
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Container(
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            const Color(0xFF0F0F23),
            const Color(0xFF1A1A3E),
            const Color(0xFF0F0F23),
          ],
        ),
      ),
      child: Scaffold(
        backgroundColor: Colors.transparent,
        body: SafeArea(
          child: Padding(
            padding: const EdgeInsets.all(24.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // Header
                Row(
                  children: [
                    Icon(Icons.rocket_launch,
                        color: theme.colorScheme.primary, size: 32),
                    const SizedBox(width: 12),
                    Text(
                      'Monster Engine × Flutter',
                      style: theme.textTheme.headlineMedium?.copyWith(
                        color: Colors.white,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 8),
                Text(
                  'Embedder Integration Test',
                  style: theme.textTheme.bodyLarge?.copyWith(
                    color: Colors.white54,
                  ),
                ),
                const SizedBox(height: 24),

                // Status cards
                Row(
                  children: [
                    _StatusCard(
                      title: 'STATUS',
                      value: _status,
                      icon: Icons.link,
                      color: _status == 'pong'
                          ? const Color(0xFF64FFDA)
                          : Colors.orange,
                    ),
                    const SizedBox(width: 16),
                    _StatusCard(
                      title: 'ENGINE',
                      value: _engineInfo,
                      icon: Icons.memory,
                      color: const Color(0xFF82B1FF),
                    ),
                    const SizedBox(width: 16),
                    _StatusCard(
                      title: 'TICKS',
                      value: '$_tickCount',
                      icon: Icons.timer,
                      color: const Color(0xFFFF6E40),
                    ),
                    const SizedBox(width: 16),
                    _StatusCard(
                      title: 'FPS',
                      value: '$_fps',
                      icon: Icons.speed,
                      color: const Color(0xFFFFD740),
                    ),
                  ],
                ),
                const SizedBox(height: 24),

                // Action buttons
                Row(
                  children: [
                    ElevatedButton.icon(
                      onPressed: _pingEngine,
                      icon: const Icon(Icons.wifi_tethering),
                      label: const Text('Ping Engine'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: theme.colorScheme.primary,
                        foregroundColor: Colors.black,
                      ),
                    ),
                    const SizedBox(width: 12),
                    OutlinedButton.icon(
                      onPressed: () {
                        setState(() {
                          _log.clear();
                        });
                      },
                      icon: const Icon(Icons.clear_all),
                      label: const Text('Clear Log'),
                      style: OutlinedButton.styleFrom(
                        foregroundColor: Colors.white70,
                        side: const BorderSide(color: Colors.white24),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 24),

                // Log
                Text(
                  'Activity Log',
                  style: theme.textTheme.titleMedium?.copyWith(
                    color: Colors.white70,
                  ),
                ),
                const SizedBox(height: 8),
                Expanded(
                  child: Container(
                    width: double.infinity,
                    padding: const EdgeInsets.all(16),
                    decoration: BoxDecoration(
                      color: Colors.black26,
                      borderRadius: BorderRadius.circular(12),
                      border: Border.all(color: Colors.white12),
                    ),
                    child: _log.isEmpty
                        ? Center(
                            child: Text(
                              'Waiting for messages...',
                              style: TextStyle(color: Colors.white30),
                            ),
                          )
                        : ListView.builder(
                            itemCount: _log.length,
                            itemBuilder: (context, index) {
                              return Padding(
                                padding:
                                    const EdgeInsets.symmetric(vertical: 2),
                                child: Text(
                                  '› ${_log[index]}',
                                  style: TextStyle(
                                    fontFamily: 'monospace',
                                    fontSize: 13,
                                    color: index == 0
                                        ? const Color(0xFF64FFDA)
                                        : Colors.white54,
                                  ),
                                ),
                              );
                            },
                          ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class _StatusCard extends StatelessWidget {
  final String title;
  final String value;
  final IconData icon;
  final Color color;

  const _StatusCard({
    required this.title,
    required this.value,
    required this.icon,
    required this.color,
  });

  @override
  Widget build(BuildContext context) {
    return Expanded(
      child: Container(
        padding: const EdgeInsets.all(16),
        decoration: BoxDecoration(
          color: color.withOpacity(0.08),
          borderRadius: BorderRadius.circular(12),
          border: Border.all(color: color.withOpacity(0.3)),
        ),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(icon, color: color, size: 16),
                const SizedBox(width: 6),
                Text(
                  title,
                  style: TextStyle(
                    color: color.withOpacity(0.7),
                    fontSize: 11,
                    fontWeight: FontWeight.bold,
                    letterSpacing: 1.2,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Text(
              value,
              style: TextStyle(
                color: Colors.white,
                fontSize: 18,
                fontWeight: FontWeight.w600,
              ),
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            ),
          ],
        ),
      ),
    );
  }
}
