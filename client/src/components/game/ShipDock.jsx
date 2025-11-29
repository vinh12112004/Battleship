import React from "react";
import {
    CarrierShip,
    Battleship,
    Destroyer,
    Submarine,
    PatrolBoat,
} from "./Ships";

export default function ShipDock({ placedShips = [], onShipDragStart }) {
    const ships = [
        {
            id: "carrier",
            name: "Aircraft Carrier",
            size: 5,
            component: CarrierShip,
            color: "#FF6B35",
        },
        {
            id: "battleship",
            name: "Battleship",
            size: 4,
            component: Battleship,
            color: "#FFB800",
        },
        {
            id: "destroyer",
            name: "Destroyer",
            size: 3,
            component: Destroyer,
            color: "#00FF88",
        },
        {
            id: "submarine",
            name: "Submarine",
            size: 3,
            component: Submarine,
            color: "#00D9FF",
        },
        {
            id: "patrol",
            name: "Patrol Boat",
            size: 2,
            component: PatrolBoat,
            color: "#00D9FF",
        },
    ];

    const handleDragStart = (e, ship) => {
        if (onShipDragStart) {
            onShipDragStart(ship);
        }
        e.dataTransfer.effectAllowed = "move";
        e.dataTransfer.setData(
            "ship",
            JSON.stringify({ id: ship.id, size: ship.size })
        );
    };

    const ShipComponent = ({ ship }) => {
        const Component = ship.component;
        return (
            <div className="flex items-center justify-center">
                <Component
                    style={{
                        color: ship.color,
                        transform: "scale(0.6)",
                        transformOrigin: "top left",
                    }}
                />
            </div>
        );
    };

    return (
        <div className="bg-transparent border border-[#00d9ff] border-opacity-30 rounded-lg p-6">
            <h2 className="text-2xl font-bold text-[#00d9ff] mb-4">
                Ship Dock
            </h2>
            <p className="text-[#00d9ff] text-opacity-70 text-sm mb-4">
                Drag ships to your board ({placedShips.length}/{ships.length}{" "}
                placed)
            </p>

            <div className="flex flex-row items-center gap-3 overflow-x-auto py-2">
                {ships.map((ship) => {
                    const isPlaced = placedShips.includes(ship.id);
                    return (
                        <div
                            key={ship.id}
                            draggable={!isPlaced}
                            onDragStart={(e) => handleDragStart(e, ship)}
                            className={`
                                bg-transparent border border-[#00d9ff] border-opacity-20 rounded p-3
                                transition cursor-move hover:border-opacity-50 hover:bg-white/5
                                ${
                                    isPlaced
                                        ? "opacity-30 cursor-not-allowed"
                                        : ""
                                }
                            `}
                        >
                            <div className="flex items-center justify-between mb-2">
                                <div>
                                    <h3 className="text-[#00d9ff] font-semibold text-sm">
                                        {ship.name}
                                    </h3>
                                    <p className="text-[#00d9ff] text-opacity-60 text-xs">
                                        Size: {ship.size} cells
                                    </p>
                                </div>
                                {isPlaced && (
                                    <span className="text-green-400">✓</span>
                                )}
                            </div>
                            <div className="flex items-center justify-center">
                                <ShipComponent ship={ship} />
                            </div>
                        </div>
                    );
                })}
            </div>
        </div>
    );
}
