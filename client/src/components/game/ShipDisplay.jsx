import React from 'react';
import {
  CarrierShip,
  Battleship,
  Destroyer,
  Submarine,
  PatrolBoat
} from './Ships.jsx';
const ShipDisplay = () => {
  return (
    <div className="flex gap-4 flex-wrap justify-center mt-6">
      <CarrierShip />
      <Battleship />
      <Destroyer />
      <Submarine />
      <PatrolBoat />
    </div>
  );
};
export default ShipDisplay;