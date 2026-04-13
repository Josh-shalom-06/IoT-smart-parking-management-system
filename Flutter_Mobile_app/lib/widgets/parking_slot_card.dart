import 'package:flutter/material.dart';
import '../models/parking_slot.dart';

class ParkingSlotCard extends StatelessWidget {
  final ParkingSlot slot;
  final String currentUserRfid;
  final VoidCallback onReserve;
  final VoidCallback onCancel;

  const ParkingSlotCard({
    Key? key,
    required this.slot,
    required this.currentUserRfid,
    required this.onReserve,
    required this.onCancel,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    Color cardColor;
    Color textColor = Colors.white;
    IconData icon;
    String statusText;
    bool isMyReservation = slot.reservedBy == currentUserRfid;

    if (slot.isOccupied) {
      cardColor = Colors.red;
      icon = Icons.car_rental;
      statusText = 'Occupied';
    } else if (slot.isReserved) {
      cardColor = isMyReservation ? Colors.blue : Colors.orange;
      icon = Icons.bookmark;
      statusText = isMyReservation ? 'My Reservation' : 'Reserved';
    } else if (slot.isAvailable) {
      cardColor = Colors.green;
      icon = Icons.local_parking;
      statusText = 'Available';
    } else {
      cardColor = Colors.grey;
      icon = Icons.help;
      statusText = 'Unknown';
    }

    return Card(
      elevation: 4,
      color: cardColor,
      child: InkWell(
        onTap: slot.isAvailable ? onReserve : (isMyReservation ? onCancel : null),
        borderRadius: BorderRadius.circular(8),
        child: Padding(
          padding: EdgeInsets.all(8), // Reduced padding from 12 to 8
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            mainAxisSize: MainAxisSize.min, // Added to minimize space usage
            children: [
              Icon(
                icon,
                size: 32, // Reduced icon size from 40 to 32
                color: textColor,
              ),
              SizedBox(height: 4), // Reduced spacing from 8 to 4
              Flexible( // Wrapped text in Flexible to prevent overflow
                child: Text(
                  '${slot.slotId}',
                  style: TextStyle(
                    fontSize: 14, // Reduced font size from 16 to 14
                    fontWeight: FontWeight.bold,
                    color: textColor,
                  ),
                  overflow: TextOverflow.ellipsis, // Handle text overflow
                ),
              ),
              SizedBox(height: 2), // Reduced spacing from 4 to 2
              Flexible( // Wrapped text in Flexible
                child: Text(
                  statusText,
                  style: TextStyle(
                    fontSize: 10, // Kept the same size
                    color: textColor,
                  ),
                  overflow: TextOverflow.ellipsis,
                ),
              ),
              if (slot.isAvailable) ...[
                SizedBox(height: 4), // Reduced spacing from 8 to 4
                Container(
                  padding: EdgeInsets.symmetric(horizontal: 6, vertical: 2), // Reduced padding
                  decoration: BoxDecoration(
                    color: Colors.white.withOpacity(0.2),
                    borderRadius: BorderRadius.circular(8), // Reduced border radius
                  ),
                  child: Text(
                    'Tap to Reserve',
                    style: TextStyle(
                      fontSize: 8, // Reduced font size from 10 to 8
                      color: textColor,
                    ),
                  ),
                ),
              ],
              if (isMyReservation) ...[
                SizedBox(height: 4), // Reduced spacing from 8 to 4
                Container(
                  padding: EdgeInsets.symmetric(horizontal: 6, vertical: 2), // Reduced padding
                  decoration: BoxDecoration(
                    color: Colors.white.withOpacity(0.2),
                    borderRadius: BorderRadius.circular(8), // Reduced border radius
                  ),
                  child: Text(
                    'Tap to Cancel',
                    style: TextStyle(
                      fontSize: 8, // Reduced font size from 10 to 8
                      color: textColor,
                    ),
                  ),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }
}