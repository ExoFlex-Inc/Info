import React, { createContext, useContext, useState, ReactNode } from "react";

interface AvatarContextType {
    avatarUrl: string;
    setAvatarUrl: (url: string) => void;
}

const AvatarContext = createContext<AvatarContextType | undefined>(undefined);

export const AvatarProvider: React.FC<{ children: ReactNode }> = ({ children }) => {
    const [avatarUrl, setAvatarUrl] = useState('');

    return (
        <AvatarContext.Provider value={{ avatarUrl, setAvatarUrl }}>
            {children}
        </AvatarContext.Provider>
    );
};

export const useAvatarContext = () => {
    const context = useContext(AvatarContext);
    if (!context) {
        throw new Error("useAvatarContext must be used within an AvatarProvider");
    }
    return context;
};