from django.db import models

class ParkingSlot(models.Model):
    slot_number = models.IntegerField(unique=True)
    status = models.CharField(max_length=20, default='Available')  # Available, Reserved, Occupied
    reserved = models.BooleanField(default=False)
    reserved_by = models.CharField(max_length=100, blank=True, null=True)
