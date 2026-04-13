class User {
  final String rfidUid;
  final String name;
  final String mobileNumber;

  User({
    required this.rfidUid,
    required this.name,
    required this.mobileNumber,
  });

  factory User.fromMap(String rfidUid, Map<dynamic, dynamic> map) {
    return User(
      rfidUid: rfidUid,
      name: map['name'] ?? '',
      mobileNumber: map['mobileNumber'] ?? '',
    );
  }
}
