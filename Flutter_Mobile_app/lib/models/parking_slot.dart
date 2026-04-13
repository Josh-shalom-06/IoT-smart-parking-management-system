class ParkingSlot {
  final String slotId;
  final String status;
  final bool reserved;
  final String? reservedBy;
  final int? lastUpdated;
  final double? distance;

  ParkingSlot({
    required this.slotId,
    required this.status,
    required this.reserved,
    this.reservedBy,
    this.lastUpdated,
    this.distance,
  });

  factory ParkingSlot.fromMap(String id, Map<dynamic, dynamic> map) {
    return ParkingSlot(
      slotId: id,
      status: map['status'] ?? 'Available',
      reserved: map['reserved'] ?? false,
      reservedBy: map['reserved_by'],
      lastUpdated: map['lastUpdated'],
      distance: map['distance']?.toDouble(),
    );
  }

  bool get isAvailable => status == 'Available' && !reserved;
  bool get isOccupied => status == 'Occupied';
  bool get isReserved => reserved;
}
