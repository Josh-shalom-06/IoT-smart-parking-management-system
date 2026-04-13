import 'package:flutter/material.dart';
import 'package:firebase_database/firebase_database.dart';
import '../models/parking_slot.dart';
import '../widgets/parking_slot_card.dart';

class ParkingSlotsScreen extends StatefulWidget {
  final String userRfidUid;

  const ParkingSlotsScreen({Key? key, required this.userRfidUid}) : super(key: key);

  @override
  _ParkingSlotsScreenState createState() => _ParkingSlotsScreenState();
}

class _ParkingSlotsScreenState extends State<ParkingSlotsScreen> {
  late DatabaseReference _slotsRef;
  late DatabaseReference _usersRef;
  String userName = '';

  @override
  void initState() {
    super.initState();
    _slotsRef = FirebaseDatabase.instance.ref('slots');
    _usersRef = FirebaseDatabase.instance.ref('rfid_users');
    _getUserName();
  }

  void _getUserName() async {
    final snapshot = await _usersRef.child(widget.userRfidUid).get();
    if (snapshot.exists) {
      final userData = snapshot.value as Map<dynamic, dynamic>;
      setState(() {
        userName = userData['name'] ?? '';
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Parking Slots'),
        backgroundColor: Colors.blue,
        actions: [
          IconButton(
            icon: Icon(Icons.logout),
            onPressed: () {
              Navigator.pushReplacementNamed(context, '/login');
            },
          ),
        ],
      ),
      body: Column(
        children: [
          Container(
            width: double.infinity,
            padding: EdgeInsets.all(16),
            color: Colors.blue.shade50,
            child: Column(
              children: [
                Text(
                  'Welcome, $userName',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
                Text(
                  'RFID: ${widget.userRfidUid}',
                  style: TextStyle(fontSize: 14, color: Colors.grey.shade600),
                ),
              ],
            ),
          ),
          Expanded(
            child: StreamBuilder(
              stream: _slotsRef.onValue,
              builder: (context, AsyncSnapshot<DatabaseEvent> snapshot) {
                if (snapshot.hasData && snapshot.data!.snapshot.value != null) {
                  Map<dynamic, dynamic> slotsData = 
                      snapshot.data!.snapshot.value as Map<dynamic, dynamic>;
                  
                  List<ParkingSlot> slots = [];
                  slotsData.forEach((key, value) {
                    if (value != null) {
                      slots.add(ParkingSlot.fromMap(key.toString(), value as Map<dynamic, dynamic>));
                    }
                  });
                  
                  // Sort slots by slot ID
                  slots.sort((a, b) => a.slotId.compareTo(b.slotId));
                  
                  return Column(
                    children: [
                      _buildSummary(slots),
                      Expanded(
                        child: GridView.builder(
                          padding: EdgeInsets.all(16),
                          gridDelegate: SliverGridDelegateWithFixedCrossAxisCount(
                            crossAxisCount: 2,
                            crossAxisSpacing: 16,
                            mainAxisSpacing: 16,
                            childAspectRatio: 1.1,
                          ),
                          itemCount: slots.length,
                          itemBuilder: (context, index) {
                            return ParkingSlotCard(
                              slot: slots[index],
                              currentUserRfid: widget.userRfidUid,
                              onReserve: () => _reserveSlot(slots[index].slotId),
                              onCancel: () => _cancelReservation(slots[index].slotId),
                            );
                          },
                        ),
                      ),
                    ],
                  );
                }
                return Center(child: CircularProgressIndicator());
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildSummary(List<ParkingSlot> slots) {
    int available = slots.where((s) => s.isAvailable).length;
    int occupied = slots.where((s) => s.isOccupied).length;
    int reserved = slots.where((s) => s.isReserved && !s.isOccupied).length;
    int myReservations = slots.where((s) => s.reservedBy == widget.userRfidUid).length;

    return Container(
      padding: EdgeInsets.all(16),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceAround,
        children: [
          _buildSummaryItem('Available', available, Colors.green),
          _buildSummaryItem('Occupied', occupied, Colors.red),
          _buildSummaryItem('Reserved', reserved, Colors.orange),
          _buildSummaryItem('My Slots', myReservations, Colors.blue),
        ],
      ),
    );
  }

  Widget _buildSummaryItem(String label, int count, Color color) {
    return Column(
      children: [
        Container(
          width: 40,
          height: 40,
          decoration: BoxDecoration(
            color: color,
            borderRadius: BorderRadius.circular(20),
          ),
          child: Center(
            child: Text(
              '$count',
              style: TextStyle(
                color: Colors.white,
                fontWeight: FontWeight.bold,
                fontSize: 18,
              ),
            ),
          ),
        ),
        SizedBox(height: 4),
        Text(
          label,
          style: TextStyle(fontSize: 12, fontWeight: FontWeight.w500),
        ),
      ],
    );
  }

  void _reserveSlot(String slotId) async {
    try {
      await _slotsRef.child(slotId).update({
        'reserved': true,
        'reserved_by': widget.userRfidUid,
        'lastUpdated': ServerValue.timestamp,
      });
      
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Slot $slotId reserved successfully!'),
          backgroundColor: Colors.green,
        ),
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to reserve slot: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }

  void _cancelReservation(String slotId) async {
    try {
      await _slotsRef.child(slotId).update({
        'reserved': false,
        'reserved_by': null,
        'lastUpdated': ServerValue.timestamp,
      });
      
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Reservation cancelled for slot $slotId'),
          backgroundColor: Colors.orange,
        ),
      );
    } catch (e) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Failed to cancel reservation: $e'),
          backgroundColor: Colors.red,
        ),
      );
    }
  }
}