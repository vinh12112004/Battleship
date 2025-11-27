import React from "react";
import {
    CarrierShip,
    Battleship,
    Destroyer,
    Submarine,
    PatrolBoat,
} from "./Ships.jsx";

const ships = [
    { component: CarrierShip, name: "Carrier" },
    { component: Battleship, name: "Battleship" },
    { component: Destroyer, name: "Destroyer" },
    { component: Submarine, name: "Submarine" },
    { component: PatrolBoat, name: "Patrol Boat" },
];

const ShipDisplay = () => {
    return (
        <div className="flex gap-4 flex-wrap justify-center mt-6">
            {ships.map(({ component: Ship, name }, index) => (
                <div key={index} className="flex flex-col items-center">
                    <Ship />
                    <p className="mt-2 text-sm font-semibold text-[#00d9ff]">
                        {name}
                    </p>
                </div>
            ))}
        </div>
    );
};

export default ShipDisplay;
